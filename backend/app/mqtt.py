from app.config import Config


class MQTTClient:
    def __init__(self):
        self.broker = Config.MQTT_BROKER
        self.port = Config.MQTT_PORT
        self.topic = Config.MQTT_TOPIC

    def connect(self):
        """
        Placeholder MQTT connect function.
        """
        print(f"MQTT placeholder connected to {self.broker}:{self.port}")

    def publish(self, topic, message):
        """
        Placeholder MQTT publish function.
        """
        print(f"MQTT placeholder publish -> topic: {topic}, message: {message}")
        return True
