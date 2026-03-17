from datetime import datetime, timezone

from pymongo import DESCENDING, MongoClient
from pymongo.errors import PyMongoError

from app.config import Config


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

        if self.mongo_uri:
            self.client = MongoClient(self.mongo_uri)
        else:
            self.client = MongoClient(self.mongo_host, self.mongo_port)

        database = self.client[self.mongo_db]
        self.collection = database[self.mongo_collection]
        return self.collection

    def _normalize_weather_payload(self, data):
        timestamp = int(datetime.now(timezone.utc).timestamp())
        return {
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

    def save_weather_update(self, data):
        document = self._normalize_weather_payload(data)
        document_to_insert = dict(document)

        try:
            collection = self._get_collection()
            result = collection.insert_one(document_to_insert)
        except PyMongoError as exc:
            return {
                "status": "error",
                "message": f"Database write failed: {exc}",
                "data": document,
            }

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
            return {
                "status": "error",
                "message": f"Database read failed: {exc}",
            }

        if latest is None:
            return {
                "status": "empty",
                "message": "No weather data available",
            }

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
            return {
                "status": "error",
                "message": f"Database read failed: {exc}",
            }

        if not documents:
            return {
                "status": "empty",
                "message": "No weather data available",
            }

        documents.reverse()
        return documents
