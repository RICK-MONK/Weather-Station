import logging
from datetime import datetime, timezone

from pymongo import DESCENDING, MongoClient
from pymongo.errors import PyMongoError

from app.config import Config


logger = logging.getLogger(__name__)


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

    def get_recent_weather(self, limit=50):
        try:
            limit = max(1, min(int(limit), 200))
        except (TypeError, ValueError):
            limit = 50

        try:
            collection = self._get_collection()
            cursor = collection.find().sort([("timestamp", DESCENDING), ("_id", DESCENDING)]).limit(limit)
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
