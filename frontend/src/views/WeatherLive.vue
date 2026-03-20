<template>
  <v-main class="page-shell">
    <v-container class="page-container">
      <section class="page-hero">
        <div>
          <p class="page-kicker">Live</p>
          <h2 class="page-title">{{ config.title }}</h2>
          <p class="page-copy">{{ config.description }}</p>
        </div>

        <div class="hero-card">
          <p class="hero-label">Samples loaded</p>
          <p class="hero-value">{{ weatherStore.recentReadings.length }}</p>
          <p class="hero-meta">{{ weatherStore.error || 'Updated every 5 seconds' }}</p>
        </div>
      </section>

      <nav class="sensor-nav">
        <RouterLink v-for="sensor in sensors" :key="sensor.key" :to="`/live/${sensor.key}`" class="sensor-tab" active-class="sensor-tab--active">
          {{ sensor.label }}
        </RouterLink>
      </nav>

      <section class="chart-stack">
        <article class="chart-panel">
          <div class="chart-header">
            <div>
              <p class="chart-eyebrow">Recent history</p>
              <h3 class="chart-title">{{ config.chartTitle }}</h3>
            </div>
          </div>
          <div class="legend-row">
            <span v-for="series in activeSeries" :key="series.label" class="legend-item">
              <span class="legend-dot" :style="{ background: series.color }"></span>{{ series.label }}
            </span>
          </div>
          <svg class="chart-svg" viewBox="0 0 900 260" preserveAspectRatio="none">
            <polyline
              v-for="series in activeSeries"
              :key="series.label"
              :points="series.points"
              :stroke="series.color"
              fill="none"
              stroke-width="4"
              stroke-linecap="round"
              stroke-linejoin="round"
            />
          </svg>
        </article>
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
    title: 'Temperature trend monitor',
    description: 'Track how ambient temperature changes across recent gateway samples.',
    chartTitle: 'Temperature and heat response',
    series: [
      { key: 'temperature', label: 'Temperature', color: '#f4b73b' },
      { key: 'heatIndex', label: 'Heat Index', color: '#ff7a5a' },
    ],
  },
  humidity: {
    title: 'Humidity trend monitor',
    description: 'Watch relative humidity in context with temperature swing.',
    chartTitle: 'Humidity and temperature',
    series: [
      { key: 'humidity', label: 'Humidity', color: '#4a9fff' },
      { key: 'temperature', label: 'Temperature', color: '#f4b73b' },
    ],
  },
  'heat-index': {
    title: 'Heat index trend monitor',
    description: 'Observe heat stress risk with temperature and humidity context.',
    chartTitle: 'Heat index / temperature / humidity',
    series: [
      { key: 'heatIndex', label: 'Heat Index', color: '#ff7a5a' },
      { key: 'temperature', label: 'Temperature', color: '#f4b73b' },
      { key: 'humidity', label: 'Humidity', color: '#4a9fff' },
    ],
  },
  pressure: {
    title: 'Pressure trend monitor',
    description: 'Watch atmospheric pressure movement over the latest readings.',
    chartTitle: 'Pressure and estimated altitude',
    series: [
      { key: 'pressure', label: 'Pressure', color: '#4f7a50' },
      { key: 'altitude', label: 'Estimated Altitude', color: '#8d63c7' },
    ],
  },
  altitude: {
    title: 'Estimated altitude trend monitor',
    description: 'Review the derived altitude estimate in relation to pressure changes and the calibrated sea-level reference.',
    chartTitle: 'Estimated altitude and pressure',
    series: [
      { key: 'altitude', label: 'Estimated Altitude', color: '#8d63c7' },
      { key: 'pressure', label: 'Pressure', color: '#4f7a50' },
    ],
  },
  'soil-moisture': {
    title: 'Soil moisture trend monitor',
    description: 'Follow the calibrated soil moisture percentage across the recent sensor window.',
    chartTitle: 'Soil moisture percentage history',
    series: [
      { key: 'soilMoisturePercent', label: 'Soil Moisture %', color: '#00a6a6' },
    ],
  },
}

function buildSeries(readings, key, color, label) {
  if (!readings.length) {
    return { color, label, points: '' }
  }

  const values = readings.map((reading) => Number(reading[key])).filter((value) => Number.isFinite(value))
  if (!values.length) {
    return { color, label, points: '' }
  }

  const minValue = Math.min(...values)
  const maxValue = Math.max(...values)
  const spread = maxValue - minValue || 1
  const width = 900
  const height = 260
  const inset = 18
  const step = values.length > 1 ? (width - inset * 2) / (values.length - 1) : 0

  const points = values
    .map((value, index) => {
      const x = inset + step * index
      const normalized = (value - minValue) / spread
      const y = height - inset - normalized * (height - inset * 2)
      return `${x.toFixed(1)},${y.toFixed(1)}`
    })
    .join(' ')

  return { color, label, points }
}

const config = computed(() => sensorConfig[route.params.sensor] || sensorConfig.temperature)

const activeSeries = computed(() =>
  config.value.series.map((series) =>
    buildSeries(weatherStore.recentReadings, series.key, series.color, series.label),
  ),
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
.page-shell {
  min-height: 100vh;
  background: linear-gradient(180deg, #eef3f8 0%, #e8eef5 100%);
}

.page-container {
  padding-top: 40px;
  padding-bottom: 40px;
}

.page-hero {
  display: grid;
  grid-template-columns: 1.8fr 1fr;
  gap: 20px;
  margin-bottom: 20px;
}

.page-kicker,
.chart-eyebrow {
  margin: 0 0 8px;
  color: #7f91a5;
  font-size: 0.78rem;
  font-weight: 700;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.page-title,
.chart-title {
  margin: 0;
  color: #172437;
}

.page-copy {
  max-width: 56ch;
  color: #536981;
  line-height: 1.6;
}

.hero-card,
.chart-panel {
  padding: 24px;
  border: 1px solid rgba(23, 36, 55, 0.08);
  border-radius: 28px;
  background: rgba(255, 255, 255, 0.76);
  box-shadow: 0 12px 28px rgba(34, 55, 80, 0.08);
}

.hero-label {
  margin: 0;
  color: #6a7f92;
  text-transform: uppercase;
  letter-spacing: 0.12em;
  font-size: 0.78rem;
}

.hero-value {
  margin: 12px 0 6px;
  color: #172437;
  font-size: 2.6rem;
  font-weight: 800;
}

.hero-meta {
  margin: 0;
  color: #536981;
}

.sensor-nav {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-bottom: 20px;
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

.chart-stack {
  display: grid;
  gap: 20px;
}

.chart-header {
  margin-bottom: 14px;
}

.legend-row {
  display: flex;
  flex-wrap: wrap;
  gap: 14px;
  margin-bottom: 16px;
}

.legend-item {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  color: #5b7085;
  font-size: 0.88rem;
  font-weight: 600;
}

.legend-dot {
  width: 10px;
  height: 10px;
  border-radius: 999px;
}

.chart-svg {
  width: 100%;
  height: 260px;
  border-radius: 18px;
  background:
    linear-gradient(to bottom, rgba(47, 72, 98, 0.05), rgba(47, 72, 98, 0.02)),
    repeating-linear-gradient(
      to bottom,
      transparent 0,
      transparent 42px,
      rgba(23, 36, 55, 0.08) 43px
    );
}

@media (max-width: 960px) {
  .page-hero {
    grid-template-columns: 1fr;
  }
}
</style>
