import logging
from collections import deque
from datetime import datetime, timezone

from pymongo import DESCENDING, MongoClient
from pymongo.errors import PyMongoError

from app.config import Config


logger = logging.getLogger(__name__)

ANALYSIS_METRICS = (
    ("temperature", "temperature"),
    ("humidity", "humidity"),
    ("heatIndex", "heatIndex"),
    ("pressure", "pressure"),
    ("altitude", "altitude"),
    ("soilMoisturePercent", "soilMoisturePercent"),
)


class DB:
    def __init__(self):
        self.mongo_uri = Config.MONGO_URI
        self.mongo_host = Config.MONGO_HOST
        self.mongo_port = Config.MONGO_PORT
        self.mongo_db = Config.MONGO_DB
        self.mongo_collection = Config.MONGO_COLLECTION
        self.client = None
        self.collection = None

    def _get_collection(self):
        if self.collection is not None:
            return self.collection

        client_options = {"serverSelectionTimeoutMS": 5000}
        if self.mongo_uri:
            logger.info("Connecting to MongoDB using MONGO_URI")
            self.client = MongoClient(self.mongo_uri, **client_options)
        else:
            logger.info(
                "Connecting to MongoDB at %s:%s",
                self.mongo_host,
                self.mongo_port,
            )
            self.client = MongoClient(
                self.mongo_host,
                self.mongo_port,
                **client_options,
            )

        self.client.admin.command("ping")

        database = self.client[self.mongo_db]
        self.collection = database[self.mongo_collection]
        logger.info(
            "MongoDB ready on %s.%s",
            self.mongo_db,
            self.mongo_collection,
        )
        return self.collection

    def _normalize_weather_payload(self, data):
        timestamp = int(datetime.now(timezone.utc).timestamp())
        document = {
            "id": str(data["id"]),
            "type": str(data["type"]),
            "temperature": float(data["temperature"]),
            "humidity": float(data["humidity"]),
            "heatIndex": float(data["heatIndex"]),
            "pressure": float(data["pressure"]),
            "altitude": float(data["altitude"]),
            "soilMoisture": int(data["soilMoisture"]),
            "timestamp": timestamp,
        }

        if "seaLevelPressureHpa" in data:
            document["seaLevelPressureHpa"] = float(data["seaLevelPressureHpa"])

        if "altitudeEstimated" in data:
            document["altitudeEstimated"] = bool(int(data["altitudeEstimated"]))

        if "soilRaw" in data:
            document["soilRaw"] = int(data["soilRaw"])

        if "soilMoisturePercent" in data:
            document["soilMoisturePercent"] = int(data["soilMoisturePercent"])

        if "dhtOk" in data:
            document["dhtOk"] = bool(int(data["dhtOk"]))

        if "bmpOk" in data:
            document["bmpOk"] = bool(int(data["bmpOk"]))

        if "soilOk" in data:
            document["soilOk"] = bool(int(data["soilOk"]))

        if "sampleMillis" in data:
            document["sampleMillis"] = int(data["sampleMillis"])

        return document

    def save_weather_update(self, data):
        document = self._normalize_weather_payload(data)
        document_to_insert = dict(document)

        try:
            collection = self._get_collection()
            result = collection.insert_one(document_to_insert)
        except PyMongoError as exc:
            logger.exception("MongoDB write failed")
            return {
                "status": "error",
                "message": f"Database write failed: {exc}",
                "data": document,
            }

        logger.info(
            "MongoDB insert complete id=%s device=%s timestamp=%s",
            result.inserted_id,
            document["id"],
            document["timestamp"],
        )

        return {
            "status": "complete",
            "message": "Weather update saved",
            "inserted_id": str(result.inserted_id),
            "data": document,
        }

    def get_latest_weather(self):
        try:
            collection = self._get_collection()
            latest = collection.find_one(sort=[("timestamp", DESCENDING), ("_id", DESCENDING)])
        except PyMongoError as exc:
            logger.exception("MongoDB read for latest weather failed")
            return {
                "status": "error",
                "message": f"Database read failed: {exc}",
            }

        if latest is None:
            logger.info("MongoDB latest weather query returned no documents")
            return {
                "status": "empty",
                "message": "No weather data available",
            }

        logger.info(
            "MongoDB latest weather id=%s timestamp=%s",
            latest.get("id"),
            latest.get("timestamp"),
        )
        latest["_id"] = str(latest["_id"])
        return latest

    def _build_time_filter(self, start_ts=None, end_ts=None):
        query = {}

        try:
            start_ts = int(start_ts) if start_ts is not None and start_ts != "" else None
        except (TypeError, ValueError):
            start_ts = None

        try:
            end_ts = int(end_ts) if end_ts is not None and end_ts != "" else None
        except (TypeError, ValueError):
            end_ts = None

        if start_ts is None and end_ts is None:
            return query

        query["timestamp"] = {}
        if start_ts is not None:
            query["timestamp"]["$gte"] = start_ts
        if end_ts is not None:
            query["timestamp"]["$lte"] = end_ts

        return query

    def get_recent_weather(self, limit=50, start_ts=None, end_ts=None):
        try:
            limit = max(1, min(int(limit), 2000))
        except (TypeError, ValueError):
            limit = 50

        try:
            collection = self._get_collection()
            query = self._build_time_filter(start_ts=start_ts, end_ts=end_ts)
            cursor = collection.find(query).sort([("timestamp", DESCENDING), ("_id", DESCENDING)]).limit(limit)
            documents = []

            for document in cursor:
                document["_id"] = str(document["_id"])
                documents.append(document)
        except PyMongoError as exc:
            logger.exception("MongoDB recent weather query failed")
            return {
                "status": "error",
                "message": f"Database read failed: {exc}",
            }

        if not documents:
            logger.info("MongoDB recent weather query returned no documents")
            return {
                "status": "empty",
                "message": "No weather data available",
            }

        logger.info("MongoDB recent weather query returned %s documents", len(documents))
        documents.reverse()
        return documents

    def get_weather_analysis(self, start_ts=None, end_ts=None, sample_limit=240, table_limit=10):
        try:
            sample_limit = max(10, min(int(sample_limit), 1000))
        except (TypeError, ValueError):
            sample_limit = 240

        try:
            table_limit = max(1, min(int(table_limit), 50))
        except (TypeError, ValueError):
            table_limit = 10

        try:
            collection = self._get_collection()
            query = self._build_time_filter(start_ts=start_ts, end_ts=end_ts)
            total_count = collection.count_documents(query)
        except PyMongoError as exc:
            logger.exception("MongoDB analysis count query failed")
            return {
                "status": "error",
                "message": f"Database read failed: {exc}",
            }

        if total_count == 0:
            logger.info("MongoDB analysis query returned no documents")
            return {
                "status": "empty",
                "message": "No weather data available for the selected range",
            }

        target_indexes = None
        if total_count > sample_limit:
            step = (total_count - 1) / float(sample_limit - 1)
            target_indexes = {int(round(index * step)) for index in range(sample_limit)}

        stats = {
            metric: {"min": None, "max": None, "sum": 0.0, "count": 0}
            for metric, _ in ANALYSIS_METRICS
        }
        sampled_readings = []
        recent_rows = deque(maxlen=table_limit)
        range_start = None
        range_end = None

        try:
            cursor = collection.find(query).sort([("timestamp", 1), ("_id", 1)])

            for index, document in enumerate(cursor):
                document["_id"] = str(document["_id"])
                timestamp = document.get("timestamp")

                if range_start is None:
                    range_start = timestamp
                range_end = timestamp

                soil_percent = document.get("soilMoisturePercent", document.get("soilMoisture"))
                metric_values = {
                    "temperature": document.get("temperature"),
                    "humidity": document.get("humidity"),
                    "heatIndex": document.get("heatIndex"),
                    "pressure": document.get("pressure"),
                    "altitude": document.get("altitude"),
                    "soilMoisturePercent": soil_percent,
                }

                for metric, raw_value in metric_values.items():
                    try:
                        value = float(raw_value)
                    except (TypeError, ValueError):
                        continue

                    if stats[metric]["min"] is None or value < stats[metric]["min"]:
                        stats[metric]["min"] = value
                    if stats[metric]["max"] is None or value > stats[metric]["max"]:
                        stats[metric]["max"] = value
                    stats[metric]["sum"] += value
                    stats[metric]["count"] += 1

                if target_indexes is None or index in target_indexes:
                    sampled_readings.append(document)

                recent_rows.appendleft(document)
        except PyMongoError as exc:
            logger.exception("MongoDB analysis cursor failed")
            return {
                "status": "error",
                "message": f"Database read failed: {exc}",
            }

        formatted_stats = {}
        for metric, _ in ANALYSIS_METRICS:
            metric_stat = stats[metric]
            if metric_stat["count"] == 0:
                formatted_stats[metric] = {
                    "min": None,
                    "max": None,
                    "avg": None,
                }
                continue

            formatted_stats[metric] = {
                "min": metric_stat["min"],
                "max": metric_stat["max"],
                "avg": metric_stat["sum"] / metric_stat["count"],
            }

        logger.info(
            "MongoDB analysis query returned count=%s sampled=%s table_rows=%s",
            total_count,
            len(sampled_readings),
            len(recent_rows),
        )

        return {
            "status": "complete",
            "count": total_count,
            "startTimestamp": range_start,
            "endTimestamp": range_end,
            "stats": formatted_stats,
            "sampledReadings": sampled_readings,
            "recentRows": list(recent_rows),
        }
