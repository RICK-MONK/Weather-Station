<template>
  <v-main class="control-shell">
    <v-container class="control-container">
      <section class="control-hero">
        <div>
          <p class="control-kicker">Control</p>
          <h2 class="control-title">Gateway and system operations.</h2>
          <p class="control-copy">
            This page is reserved for command and operational controls. For now it surfaces system health, data freshness, and deployment notes in one place.
          </p>
        </div>
      </section>

      <v-row dense class="control-grid">
        <v-col cols="12" md="6">
          <v-card class="control-card" rounded="xl" elevation="0">
            <v-card-text>
              <p class="control-label">Gateway state</p>
              <p class="control-value">{{ gatewayState }}</p>
              <p class="control-caption">Derived from the latest backend timestamp and error state.</p>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" md="6">
          <v-card class="control-card" rounded="xl" elevation="0">
            <v-card-text>
              <p class="control-label">Polling cadence</p>
              <p class="control-value">5 seconds</p>
              <p class="control-caption">Frontend refresh interval for latest and recent data.</p>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12">
          <v-card class="control-card" rounded="xl" elevation="0">
            <v-card-text>
              <p class="control-label">Operational notes</p>
              <ul class="note-list">
                <li>Display ESP32 is acting as the Wi-Fi gateway and HTTP client.</li>
                <li>MongoDB-backed metrics are live; firewall rules should remain enabled for port 5000 on the private network.</li>
                <li>Soil moisture is still using placeholder calibration values and should be finalized before presentation.</li>
                <li>Altitude is BMP280-derived and should be described as estimated unless the sea-level reference is tuned.</li>
              </ul>
            </v-card-text>
          </v-card>
        </v-col>
      </v-row>
    </v-container>
  </v-main>
</template>

<script setup>
import { computed, onMounted, onUnmounted } from 'vue'
import { useWeatherStore } from '@/store/weather'

const weatherStore = useWeatherStore()
let pollTimer

const gatewayState = computed(() => {
  if (weatherStore.error) {
    return 'Backend error'
  }

  const timestamp = weatherStore.latestRaw?.timestamp
  if (!Number.isFinite(timestamp)) {
    return 'Waiting for first reading'
  }

  const ageSeconds = Math.max(0, Math.floor(Date.now() / 1000 - timestamp))
  if (ageSeconds <= 10) {
    return 'Gateway healthy'
  }

  if (ageSeconds <= 20) {
    return 'Gateway delayed'
  }

  return 'Gateway stale'
})

onMounted(() => {
  weatherStore.refreshWeather(20)
  pollTimer = window.setInterval(() => weatherStore.refreshWeather(20), 5000)
})

onUnmounted(() => {
  window.clearInterval(pollTimer)
})
</script>

<style scoped>
.control-shell {
  min-height: 100vh;
  background: linear-gradient(180deg, #eef3f8 0%, #ece7df 100%);
}

.control-container {
  padding-top: 40px;
  padding-bottom: 40px;
}

.control-hero {
  margin-bottom: 20px;
}

.control-kicker {
  margin: 0 0 8px;
  color: #7f91a5;
  font-size: 0.78rem;
  font-weight: 700;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.control-title {
  margin: 0;
  color: #172437;
}

.control-copy {
  max-width: 60ch;
  color: #536981;
  line-height: 1.6;
}

.control-grid {
  row-gap: 16px;
}

.control-card {
  height: 100%;
  border: 1px solid rgba(23, 36, 55, 0.08);
  background: rgba(255, 255, 255, 0.8);
  box-shadow: 0 12px 28px rgba(34, 55, 80, 0.08);
}

.control-label {
  margin: 0 0 10px;
  color: #6d7f92;
  font-size: 0.82rem;
  font-weight: 700;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.control-value {
  margin: 0;
  color: #172437;
  font-size: 2rem;
  font-weight: 800;
}

.control-caption {
  margin: 10px 0 0;
  color: #5b7085;
  line-height: 1.5;
}

.note-list {
  margin: 10px 0 0;
  padding-left: 18px;
  color: #4f6478;
  line-height: 1.7;
}
</style>
