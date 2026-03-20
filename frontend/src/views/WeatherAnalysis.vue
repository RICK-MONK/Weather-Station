<template>
  <v-main class="analysis-shell">
    <v-container class="analysis-container">
      <section class="analysis-hero">
        <div>
          <p class="analysis-kicker">Analysis</p>
          <h2 class="analysis-title">{{ config.title }}</h2>
          <p class="analysis-copy">{{ config.description }}</p>
        </div>
      </section>

      <nav class="sensor-nav">
        <RouterLink v-for="sensor in sensors" :key="sensor.key" :to="`/analysis/${sensor.key}`" class="sensor-tab" active-class="sensor-tab--active">
          {{ sensor.label }}
        </RouterLink>
      </nav>

      <v-row dense class="summary-grid">
        <v-col cols="12" md="6" lg="4">
          <v-card class="summary-card" rounded="xl" elevation="0">
            <v-card-text>
              <p class="summary-label">Minimum</p>
              <p class="summary-value">{{ formatNumber(stats?.min, config.unit, config.digits) }}</p>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" md="6" lg="4">
          <v-card class="summary-card" rounded="xl" elevation="0">
            <v-card-text>
              <p class="summary-label">Average</p>
              <p class="summary-value">{{ formatNumber(stats?.avg, config.unit, config.digits) }}</p>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" md="6" lg="4">
          <v-card class="summary-card" rounded="xl" elevation="0">
            <v-card-text>
              <p class="summary-label">Maximum</p>
              <p class="summary-value">{{ formatNumber(stats?.max, config.unit, config.digits) }}</p>
            </v-card-text>
          </v-card>
        </v-col>
      </v-row>

      <section class="table-panel">
        <div class="table-header">
          <div>
            <p class="analysis-kicker">Recent points</p>
            <h3 class="table-title">{{ config.tableTitle }}</h3>
          </div>
        </div>

        <div class="table-wrap">
          <table class="reading-table">
            <thead>
              <tr>
                <th>Time</th>
                <th>{{ config.columnLabel }}</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="reading in recentRows" :key="reading.timestamp">
                <td>{{ reading.time }}</td>
                <td>{{ reading.value }}</td>
              </tr>
            </tbody>
          </table>
        </div>
      </section>
    </v-container>
  </v-main>
</template>

<script setup>
import { computed, onMounted, onUnmounted } from 'vue'
import { useRoute } from 'vue-router'
import { useWeatherStore } from '@/store/weather'

const weatherStore = useWeatherStore()
const route = useRoute()
let pollTimer

const sensors = [
  { key: 'temperature', label: 'Temperature' },
  { key: 'humidity', label: 'Humidity' },
  { key: 'heat-index', label: 'Heat Index' },
  { key: 'pressure', label: 'Pressure' },
  { key: 'altitude', label: 'Estimated Altitude' },
  { key: 'soil-moisture', label: 'Soil Moisture' },
]

const sensorConfig = {
  temperature: {
    title: 'Temperature analysis window',
    description: 'Review the min, average, and max temperature across the latest stored readings.',
    key: 'temperature',
    unit: 'C',
    digits: 1,
    tableTitle: 'Temperature samples',
    columnLabel: 'Temperature',
  },
  humidity: {
    title: 'Humidity analysis window',
    description: 'Inspect how the relative humidity range is behaving over the recent capture window.',
    key: 'humidity',
    unit: '%',
    digits: 1,
    tableTitle: 'Humidity samples',
    columnLabel: 'Humidity',
  },
  'heat-index': {
    title: 'Heat index analysis window',
    description: 'Summarize heat index movement across the latest stored readings.',
    key: 'heatIndex',
    unit: 'C',
    digits: 1,
    tableTitle: 'Heat index samples',
    columnLabel: 'Heat Index',
  },
  pressure: {
    title: 'Pressure analysis window',
    description: 'Summarize atmospheric pressure range and stability.',
    key: 'pressure',
    unit: 'hPa',
    digits: 1,
    tableTitle: 'Pressure samples',
    columnLabel: 'Pressure',
  },
  altitude: {
    title: 'Estimated altitude analysis window',
    description: 'Review the altitude estimate derived from pressure using the calibrated sea-level reference.',
    key: 'altitude',
    unit: 'm',
    digits: 1,
    tableTitle: 'Estimated altitude samples',
    columnLabel: 'Estimated Altitude',
  },
  'soil-moisture': {
    title: 'Soil moisture analysis window',
    description: 'Track the calibrated soil moisture percentage across the recent readings.',
    key: 'soilMoisturePercent',
    unit: '%',
    digits: 0,
    tableTitle: 'Soil moisture percentage samples',
    columnLabel: 'Soil Moisture %',
  },
}

function average(values) {
  if (!values.length) {
    return null
  }

  return values.reduce((sum, value) => sum + value, 0) / values.length
}

function formatNumber(value, unit, digits = 1) {
  if (!Number.isFinite(value)) {
    return '--'
  }

  return `${value.toFixed(digits)} ${unit}`.trim()
}

const config = computed(() => sensorConfig[route.params.sensor] || sensorConfig.temperature)

const stats = computed(() => {
  const values = weatherStore.recentReadings
    .map((reading) => Number(reading[config.value.key]))
    .filter((value) => Number.isFinite(value))

  if (!values.length) {
    return null
  }

  return {
    min: Math.min(...values),
    max: Math.max(...values),
    avg: average(values),
  }
})

const recentRows = computed(() =>
  weatherStore.recentReadings
    .slice(-10)
    .reverse()
    .map((reading) => ({
      timestamp: reading.timestamp,
      time: new Date(reading.timestamp * 1000).toLocaleTimeString(),
      value: formatNumber(Number(reading[config.value.key]), config.value.unit, config.value.digits),
    })),
)

onMounted(() => {
  weatherStore.refreshWeather(40)
  pollTimer = window.setInterval(() => weatherStore.refreshWeather(40), 5000)
})

onUnmounted(() => {
  window.clearInterval(pollTimer)
})
</script>

<style scoped>
.analysis-shell {
  min-height: 100vh;
  background: linear-gradient(180deg, #f4f1ec 0%, #e8eef5 100%);
}

.analysis-container {
  padding-top: 40px;
  padding-bottom: 40px;
}

.analysis-hero,
.table-panel,
.summary-card {
  border: 1px solid rgba(23, 36, 55, 0.08);
  border-radius: 28px;
  background: rgba(255, 255, 255, 0.8);
  box-shadow: 0 12px 28px rgba(34, 55, 80, 0.08);
}

.analysis-hero,
.table-panel {
  padding: 24px;
}

.analysis-kicker {
  margin: 0 0 8px;
  color: #8a9aae;
  font-size: 0.78rem;
  font-weight: 700;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.analysis-title,
.table-title {
  margin: 0;
  color: #172437;
}

.analysis-copy {
  max-width: 62ch;
  color: #536981;
  line-height: 1.6;
}

.sensor-nav {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin: 20px 0;
}

.sensor-tab {
  padding: 10px 14px;
  border-radius: 999px;
  background: rgba(36, 77, 115, 0.08);
  color: #36536f;
  font-weight: 700;
  text-decoration: none;
}

.sensor-tab--active {
  background: #244d73;
  color: #f4f8fc;
}

.summary-grid {
  margin-bottom: 20px;
  row-gap: 16px;
}

.summary-card {
  height: 100%;
}

.summary-label {
  margin: 0 0 10px;
  color: #6d7f92;
  font-size: 0.82rem;
  font-weight: 700;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.summary-value {
  margin: 0;
  color: #172437;
  font-size: 1.6rem;
  font-weight: 800;
  line-height: 1.2;
}

.table-header {
  margin-bottom: 16px;
}

.table-wrap {
  overflow-x: auto;
}

.reading-table {
  width: 100%;
  border-collapse: collapse;
}

.reading-table th,
.reading-table td {
  padding: 12px 10px;
  border-bottom: 1px solid rgba(23, 36, 55, 0.08);
  text-align: left;
  color: #284056;
}

.reading-table th {
  color: #6d7f92;
  font-size: 0.82rem;
  font-weight: 700;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}
</style>
