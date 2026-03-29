import json
import logging

import paho.mqtt.client as mqtt

from app.config import Config


logger = logging.getLogger(__name__)


class MQTTClient:
    def __init__(self):
        self.broker = Config.MQTT_BROKER
        self.port = Config.MQTT_PORT
        self.topic = Config.MQTT_TOPIC
        self.client = mqtt.Client()
        self._connected = False
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect

    def _on_connect(self, client, userdata, flags, rc, properties=None):
        self._connected = rc == 0
        if self._connected:
            logger.info("MQTT connected to %s:%s", self.broker, self.port)
        else:
            logger.error("MQTT connect failed rc=%s", rc)

    def _on_disconnect(self, client, userdata, flags, rc, properties=None):
        self._connected = False
        logger.warning("MQTT disconnected rc=%s", rc)

    def connect(self):
        if self._connected:
            return True

        try:
            self.client.connect(self.broker, self.port, keepalive=60)
            self.client.loop_start()
            logger.info("MQTT connect requested to %s:%s", self.broker, self.port)
            return True
        except Exception:
            logger.exception("MQTT connection failed")
            return False

    def publish(self, topic, message):
        if not self.connect():
            return False

        payload = message if isinstance(message, str) else json.dumps(message)

        try:
            result = self.client.publish(topic or self.topic, payload)
            result.wait_for_publish()
        except Exception:
            logger.exception("MQTT publish failed")
            return False

        if result.rc != mqtt.MQTT_ERR_SUCCESS:
            logger.error("MQTT publish failed rc=%s", result.rc)
            return False

        logger.info("MQTT publish complete topic=%s payload=%s", topic or self.topic, payload)
        return True
