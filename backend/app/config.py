import os

from dotenv import load_dotenv


load_dotenv()


class Config:
    MONGO_URI = os.getenv("MONGO_URI")
    MONGO_HOST = os.getenv("MONGO_HOST", "localhost")
    MONGO_PORT = int(os.getenv("MONGO_PORT", 27017))
    MONGO_DB = os.getenv("MONGO_DB", "ELET2415")
    MONGO_COLLECTION = os.getenv("MONGO_COLLECTION", "weather")
    MQTT_BROKER = os.getenv("MQTT_BROKER", "localhost")
    MQTT_PORT = int(os.getenv("MQTT_PORT", 1883))
    MQTT_TOPIC = os.getenv("MQTT_TOPIC", "weatherstation/update")
