<template>
  <v-app>
    <v-main class="dashboard-shell">
      <v-container class="dashboard-container">
        <section class="hero-panel">
          <div>
            <p class="eyebrow">ELET2415 Weather Station</p>
            <h1 class="hero-title">Live conditions from your ESP-NOW gateway.</h1>
            <p class="hero-copy">
              The display node is receiving sensor packets, forwarding them to Flask, and exposing the latest reading for the dashboard.
            </p>
          </div>

          <div class="status-panel">
            <div class="status-row">
              <span class="status-chip" :class="heroStatus.class">
                {{ heroStatus.text }}
              </span>
            </div>

            <div class="meta-block">
              <p class="meta-label">Last update</p>
              <p class="meta-value">{{ lastUpdatedText }}</p>
            </div>

            <div class="meta-block">
              <p class="meta-label">Reading age</p>
              <p class="meta-value">{{ readingAgeText }}</p>
            </div>
          </div>
        </section>

        <v-row class="metric-grid" dense>
          <v-col v-for="metric in metrics" :key="metric.label" cols="12" sm="6" lg="4">
            <v-card class="metric-card" rounded="xl" elevation="0">
              <v-card-text>
                <p class="metric-label">{{ metric.label }}</p>
                <p class="metric-value">{{ metric.value }}</p>

                <div class="metric-meta-row">
                  <span class="trend-pill" :class="metric.trend.class">
                    {{ metric.trend.icon }} {{ metric.trend.label }}
                  </span>
                  <span class="status-chip status-chip--small" :class="metric.status.class">
                    {{ metric.status.text }}
                  </span>
                </div>

                <p class="metric-description">{{ metric.caption }}</p>
              </v-card-text>
            </v-card>
          </v-col>
        </v-row>

        <section class="chart-grid">
          <article class="chart-panel">
            <div class="chart-header">
              <div>
                <p class="chart-eyebrow">Recent history</p>
                <h2 class="chart-title">Temperature, humidity, heat index</h2>
              </div>
            </div>
            <div class="legend-row">
              <span class="legend-item"><span class="legend-dot legend-dot--temp"></span>Temperature</span>
              <span class="legend-item"><span class="legend-dot legend-dot--humidity"></span>Humidity</span>
              <span class="legend-item"><span class="legend-dot legend-dot--heat"></span>Heat Index</span>
            </div>
            <svg class="chart-svg" viewBox="0 0 620 240" preserveAspectRatio="none">
              <polyline
                v-for="series in comfortSeries"
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

          <article class="chart-panel">
            <div class="chart-header">
              <div>
                <p class="chart-eyebrow">Recent history</p>
                <h2 class="chart-title">Pressure and soil moisture</h2>
              </div>
            </div>
            <div class="legend-row">
              <span class="legend-item"><span class="legend-dot legend-dot--pressure"></span>Pressure</span>
              <span class="legend-item"><span class="legend-dot legend-dot--soil"></span>Soil Moisture</span>
            </div>
            <svg class="chart-svg" viewBox="0 0 620 240" preserveAspectRatio="none">
              <polyline
                v-for="series in environmentSeries"
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
  </v-app>
</template>

<script setup>
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import { useWeatherStore } from '@/store/weather'

const weatherStore = useWeatherStore()
const nowMs = ref(Date.now())

let pollTimer
let clockTimer

function formatNumber(value, digits = 1) {
  return Number.isFinite(value) ? value.toFixed(digits) : '--'
}

function formatMetric(value, unit, digits = 1) {
  const formatted = formatNumber(value, digits)
  return formatted === '--' ? '--' : `${formatted} ${unit}`.trim()
}

function getTrend(current, previous, epsilon = 0.05) {
  if (!Number.isFinite(current) || !Number.isFinite(previous)) {
    return { icon: '->', label: 'No baseline', class: 'trend-neutral' }
  }

  const diff = current - previous
  if (Math.abs(diff) <= epsilon) {
    return { icon: '->', label: 'Stable', class: 'trend-neutral' }
  }

  if (diff > 0) {
    return { icon: '^', label: 'Rising', class: 'trend-up' }
  }

  return { icon: 'v', label: 'Falling', class: 'trend-down' }
}

function getStatus(value, type) {
  if (!Number.isFinite(value)) {
    return { text: 'Unknown', class: 'chip-neutral' }
  }

  switch (type) {
    case 'temperature':
      if (value >= 32) return { text: 'Hot', class: 'chip-danger' }
      if (value >= 28) return { text: 'Warm', class: 'chip-warn' }
      return { text: 'Normal', class: 'chip-good' }

    case 'humidity':
      if (value >= 75) return { text: 'Very Humid', class: 'chip-danger' }
      if (value >= 60) return { text: 'Humid', class: 'chip-warn' }
      if (value >= 40) return { text: 'Comfortable', class: 'chip-good' }
      return { text: 'Dry Air', class: 'chip-neutral' }

    case 'heatIndex':
      if (value >= 35) return { text: 'Heat Stress', class: 'chip-danger' }
      if (value >= 30) return { text: 'Elevated', class: 'chip-warn' }
      return { text: 'Comfortable', class: 'chip-good' }

    case 'pressure':
      if (value >= 1030) return { text: 'High', class: 'chip-good' }
      if (value <= 1000) return { text: 'Low', class: 'chip-warn' }
      return { text: 'Stable', class: 'chip-neutral' }

    case 'altitude':
      return { text: 'Estimated', class: 'chip-neutral' }

    case 'soilMoisture':
      if (value >= 320) return { text: 'Dry', class: 'chip-danger' }
      if (value >= 260) return { text: 'Moderate', class: 'chip-warn' }
      return { text: 'Moist', class: 'chip-good' }

    default:
      return { text: 'Unknown', class: 'chip-neutral' }
  }
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
  const width = 620
  const height = 240
  const inset = 16
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

const latestRaw = computed(() => weatherStore.latestRaw || {})
const previousRaw = computed(() => {
  const readings = weatherStore.recentReadings
  return readings.length > 1 ? readings[readings.length - 2] : null
})

const lastUpdatedText = computed(() => {
  if (!Number.isFinite(latestRaw.value.timestamp)) {
    return '--'
  }

  return new Date(latestRaw.value.timestamp * 1000).toLocaleString()
})

const readingAgeSeconds = computed(() => {
  if (!Number.isFinite(latestRaw.value.timestamp)) {
    return null
  }

  return Math.max(0, Math.floor(nowMs.value / 1000 - latestRaw.value.timestamp))
})

const readingAgeText = computed(() => {
  if (readingAgeSeconds.value === null) {
    return '--'
  }

  const seconds = readingAgeSeconds.value
  if (seconds < 60) {
    return `${seconds}s ago`
  }

  const minutes = Math.floor(seconds / 60)
  if (minutes < 60) {
    return `${minutes}m ago`
  }

  const hours = Math.floor(minutes / 60)
  return `${hours}h ago`
})

const backendFreshness = computed(() => {
  const age = readingAgeSeconds.value
  if (age === null) {
    return { text: weatherStore.loading ? 'Syncing' : 'No data', class: 'chip-neutral' }
  }

  if (age <= 10) {
    return { text: 'Live', class: 'chip-good' }
  }

  if (age <= 30) {
    return { text: 'Delayed', class: 'chip-warn' }
  }

  return { text: 'Stale', class: 'chip-danger' }
})

const heroStatus = computed(() => {
  if (weatherStore.error) {
    return { text: weatherStore.error, class: 'chip-danger' }
  }

  return backendFreshness.value
})

const metrics = computed(() => [
  {
    label: 'Temperature',
    value: formatMetric(Number(latestRaw.value.temperature), 'C'),
    caption: 'Ambient air temperature from DHT22',
    trend: getTrend(Number(latestRaw.value.temperature), Number(previousRaw.value?.temperature), 0.05),
    status: getStatus(Number(latestRaw.value.temperature), 'temperature'),
  },
  {
    label: 'Humidity',
    value: formatMetric(Number(latestRaw.value.humidity), '%'),
    caption: 'Relative humidity from DHT22',
    trend: getTrend(Number(latestRaw.value.humidity), Number(previousRaw.value?.humidity), 0.1),
    status: getStatus(Number(latestRaw.value.humidity), 'humidity'),
  },
  {
    label: 'Heat Index',
    value: formatMetric(Number(latestRaw.value.heatIndex), 'C'),
    caption: 'Calculated apparent temperature',
    trend: getTrend(Number(latestRaw.value.heatIndex), Number(previousRaw.value?.heatIndex), 0.05),
    status: getStatus(Number(latestRaw.value.heatIndex), 'heatIndex'),
  },
  {
    label: 'Atmospheric Pressure',
    value: formatMetric(Number(latestRaw.value.pressure), 'hPa'),
    caption: 'Barometric pressure from BMP280',
    trend: getTrend(Number(latestRaw.value.pressure), Number(previousRaw.value?.pressure), 0.1),
    status: getStatus(Number(latestRaw.value.pressure), 'pressure'),
  },
  {
    label: 'Estimated Altitude',
    value: formatMetric(Number(latestRaw.value.altitude), 'm'),
    caption: 'BMP280-derived altitude using pressure reference',
    trend: getTrend(Number(latestRaw.value.altitude), Number(previousRaw.value?.altitude), 0.1),
    status: getStatus(Number(latestRaw.value.altitude), 'altitude'),
  },
  {
    label: 'Soil Moisture',
    value: formatMetric(Number(latestRaw.value.soilMoisture), 'raw', 0),
    caption: 'Raw probe reading, calibration pending',
    trend: getTrend(Number(latestRaw.value.soilMoisture), Number(previousRaw.value?.soilMoisture), 1),
    status: getStatus(Number(latestRaw.value.soilMoisture), 'soilMoisture'),
  },
])

const comfortSeries = computed(() => [
  buildSeries(weatherStore.recentReadings, 'temperature', '#f4b73b', 'Temperature'),
  buildSeries(weatherStore.recentReadings, 'humidity', '#4a9fff', 'Humidity'),
  buildSeries(weatherStore.recentReadings, 'heatIndex', '#ff7a5a', 'Heat Index'),
])

const environmentSeries = computed(() => [
  buildSeries(weatherStore.recentReadings, 'pressure', '#4f7a50', 'Pressure'),
  buildSeries(weatherStore.recentReadings, 'soilMoisture', '#00a6a6', 'Soil Moisture'),
])

onMounted(() => {
  weatherStore.refreshWeather()
  pollTimer = window.setInterval(() => {
    weatherStore.refreshWeather()
  }, 5000)
  clockTimer = window.setInterval(() => {
    nowMs.value = Date.now()
  }, 1000)
})

onBeforeUnmount(() => {
  window.clearInterval(pollTimer)
  window.clearInterval(clockTimer)
})
</script>

<style scoped>
.dashboard-shell {
  min-height: 100vh;
  background:
    radial-gradient(circle at top left, rgba(255, 210, 140, 0.32), transparent 30%),
    radial-gradient(circle at top right, rgba(109, 187, 255, 0.3), transparent 26%),
    linear-gradient(180deg, #f7f2e8 0%, #e8eef5 100%);
}

.dashboard-container {
  padding-top: 40px;
  padding-bottom: 40px;
}

.hero-panel {
  display: grid;
  grid-template-columns: 1.7fr 1fr;
  gap: 24px;
  margin-bottom: 24px;
  padding: 28px;
  border: 1px solid rgba(23, 36, 55, 0.08);
  border-radius: 28px;
  background: rgba(255, 252, 247, 0.82);
  backdrop-filter: blur(12px);
  box-shadow: 0 18px 45px rgba(32, 51, 74, 0.09);
}

.eyebrow {
  margin: 0 0 12px;
  color: #9b5c22;
  font-size: 0.8rem;
  font-weight: 700;
  letter-spacing: 0.16em;
  text-transform: uppercase;
}

.hero-title {
  margin: 0;
  color: #172437;
  font-size: clamp(2rem, 4vw, 3.4rem);
  line-height: 1.02;
}

.hero-copy {
  max-width: 58ch;
  margin: 16px 0 0;
  color: #486079;
  font-size: 1rem;
  line-height: 1.6;
}

.status-panel {
  padding: 20px;
  border-radius: 22px;
  background: linear-gradient(180deg, #18334d 0%, #244d73 100%);
  color: #f4f8fc;
}

.status-row {
  display: flex;
  align-items: center;
}

.meta-block {
  margin-top: 18px;
}

.meta-label {
  margin: 0 0 6px;
  font-size: 0.72rem;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  opacity: 0.8;
}

.meta-value {
  margin: 0;
  font-size: 1.15rem;
  font-weight: 700;
}

.metric-grid {
  row-gap: 16px;
}

.metric-card {
  height: 100%;
  border: 1px solid rgba(23, 36, 55, 0.08);
  background: rgba(255, 255, 255, 0.72);
  box-shadow: 0 12px 28px rgba(34, 55, 80, 0.08);
}

.metric-label {
  margin: 0 0 12px;
  color: #6d7f92;
  font-size: 0.78rem;
  font-weight: 700;
  letter-spacing: 0.16em;
  text-transform: uppercase;
  opacity: 0.8;
}

.metric-value {
  margin: 0;
  color: #172437;
  font-size: clamp(1.7rem, 3vw, 2.6rem);
  font-weight: 800;
  line-height: 1.05;
}

.metric-meta-row {
  display: flex;
  gap: 0.5rem;
  align-items: center;
  margin: 0.75rem 0 0.5rem;
  flex-wrap: wrap;
}

.metric-description {
  margin: 0;
  color: #556b81;
  font-size: 0.95rem;
  line-height: 1.5;
  opacity: 0.9;
}

.status-chip,
.trend-pill {
  display: inline-flex;
  align-items: center;
  gap: 0.25rem;
  padding: 0.3rem 0.7rem;
  border-radius: 999px;
  font-size: 0.78rem;
  font-weight: 700;
  line-height: 1;
}

.status-chip--small {
  font-size: 0.72rem;
}

.chip-good {
  background: #dff6e8;
  color: #136c3f;
}

.chip-warn {
  background: #fff0d6;
  color: #9a5b00;
}

.chip-danger {
  background: #fde3e3;
  color: #a12626;
}

.chip-neutral {
  background: #e8edf3;
  color: #49637a;
}

.trend-up {
  background: #e8f5ec;
  color: #1d6b3f;
}

.trend-down {
  background: #fbe9e9;
  color: #9b2c2c;
}

.trend-neutral {
  background: #eef2f6;
  color: #5a6b7b;
}

.chart-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 20px;
  margin-top: 24px;
}

.chart-panel {
  padding: 24px;
  border: 1px solid rgba(23, 36, 55, 0.08);
  border-radius: 28px;
  background: rgba(255, 255, 255, 0.78);
  box-shadow: 0 12px 28px rgba(34, 55, 80, 0.08);
}

.chart-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.chart-eyebrow {
  margin: 0 0 6px;
  color: #8a9aae;
  font-size: 0.78rem;
  font-weight: 700;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.chart-title {
  margin: 0;
  color: #172437;
  font-size: 1.2rem;
}

.legend-row {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
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

.legend-dot--temp {
  background: #f4b73b;
}

.legend-dot--humidity {
  background: #4a9fff;
}

.legend-dot--heat {
  background: #ff7a5a;
}

.legend-dot--pressure {
  background: #4f7a50;
}

.legend-dot--soil {
  background: #00a6a6;
}

.chart-svg {
  width: 100%;
  height: 240px;
  border-radius: 18px;
  background:
    linear-gradient(to bottom, rgba(47, 72, 98, 0.05), rgba(47, 72, 98, 0.02)),
    repeating-linear-gradient(
      to bottom,
      transparent 0,
      transparent 38px,
      rgba(23, 36, 55, 0.08) 39px
    );
}

@media (max-width: 960px) {
  .hero-panel {
    grid-template-columns: 1fr;
  }

  .chart-grid {
    grid-template-columns: 1fr;
  }
}
</style>
