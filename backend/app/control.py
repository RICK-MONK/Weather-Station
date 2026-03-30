import threading
import uuid
from datetime import datetime, timezone


def utc_timestamp():
    return int(datetime.now(timezone.utc).timestamp())


class ControlPlane:
    def __init__(self):
        self._lock = threading.Lock()
        self._maintenance_mode = False
        self._last_acknowledged_at = None
        self._pending_commands = []
        self._recent_commands = []

    def _append_recent_command(self, command):
        self._recent_commands.append(dict(command))
        self._recent_commands = self._recent_commands[-10:]

    def _build_summary(self, latest_weather):
        latest_timestamp = None
        latest_device_id = None

        if isinstance(latest_weather, dict):
            latest_timestamp = latest_weather.get("timestamp")
            latest_device_id = latest_weather.get("id")

        return {
            "maintenanceMode": self._maintenance_mode,
            "lastAcknowledgedAt": self._last_acknowledged_at,
            "commandQueueDepth": len(self._pending_commands),
            "latestWeatherTimestamp": latest_timestamp,
            "latestDeviceId": latest_device_id,
            "pendingCommands": [dict(command) for command in self._pending_commands],
            "recentCommands": [dict(command) for command in reversed(self._recent_commands)],
            "deliveryMode": "backend-queued",
        }

    def get_status(self, latest_weather=None):
        with self._lock:
            return self._build_summary(latest_weather)

    def set_maintenance_mode(self, enabled, actor="frontend"):
        with self._lock:
            self._maintenance_mode = bool(enabled)
            event = {
                "id": str(uuid.uuid4()),
                "kind": "state-change",
                "target": "system",
                "command": "maintenance-mode",
                "status": "applied",
                "value": self._maintenance_mode,
                "actor": actor,
                "createdAt": utc_timestamp(),
            }
            self._append_recent_command(event)
            return event

    def acknowledge_alerts(self, actor="frontend"):
        with self._lock:
            self._last_acknowledged_at = utc_timestamp()
            event = {
                "id": str(uuid.uuid4()),
                "kind": "acknowledgement",
                "target": "system",
                "command": "acknowledge-alerts",
                "status": "applied",
                "actor": actor,
                "createdAt": self._last_acknowledged_at,
            }
            self._append_recent_command(event)
            return event

    def queue_command(self, target, command, actor="frontend", note=""):
        with self._lock:
            queued_command = {
                "id": str(uuid.uuid4()),
                "kind": "device-command",
                "target": str(target),
                "command": str(command),
                "status": "queued",
                "actor": actor,
                "note": str(note),
                "createdAt": utc_timestamp(),
            }
            self._pending_commands.append(queued_command)
            self._append_recent_command(queued_command)
            return queued_command

    def clear_pending_commands(self, actor="frontend"):
        with self._lock:
            cleared_count = len(self._pending_commands)

            if cleared_count:
                cleared_at = utc_timestamp()
                for command in self._pending_commands:
                    command["status"] = "cleared"
                    command["clearedAt"] = cleared_at
                    self._append_recent_command(command)

            self._pending_commands = []

            event = {
                "id": str(uuid.uuid4()),
                "kind": "queue-maintenance",
                "target": "system",
                "command": "clear-pending-commands",
                "status": "applied",
                "actor": actor,
                "clearedCount": cleared_count,
                "createdAt": utc_timestamp(),
            }
            self._append_recent_command(event)
            return event
