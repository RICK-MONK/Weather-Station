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

      <section class="filter-panel">
        <div class="filter-copy">
          <p class="analysis-kicker">Date range</p>
          <h3 class="table-title">Analyze between a start and end date.</h3>
        </div>

        <div class="filter-grid">
          <label class="filter-field">
            <span>Start date</span>
            <input v-model="startDate" class="date-input" type="date" />
          </label>

          <label class="filter-field">
            <span>End date</span>
            <input v-model="endDate" class="date-input" type="date" />
          </label>

          <div class="filter-actions">
            <button class="filter-button filter-button--primary" type="button" @click="applyDateRange">
              Apply range
            </button>
            <button class="filter-button" type="button" @click="resetDateRange">
              Reset
            </button>
          </div>
        </div>
      </section>

      <section v-if="weatherStore.analysisError || weatherStore.analysisLoading || !weatherStore.analysisData.count" class="analysis-status-panel">
        <p class="analysis-status" :class="{ 'analysis-status--error': weatherStore.analysisError }">
          {{ analysisStatusText }}
        </p>
      </section>

      <v-row dense class="summary-grid">
        <v-col cols="12" md="6" lg="4">
          <v-card class="summary-card" rounded="xl" elevation="0">
            <v-card-text>
              <p class="summary-label">Minimum</p>
              <p class="summary-value">{{ formatNumber(stats?.min, config.key, config.digits) }}</p>
              <p class="summary-meta">{{ summaryRangeText }}</p>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" md="6" lg="4">
          <v-card class="summary-card" rounded="xl" elevation="0">
            <v-card-text>
              <p class="summary-label">Average</p>
              <p class="summary-value">{{ formatNumber(stats?.avg, config.key, config.digits) }}</p>
              <p class="summary-meta">{{ summaryRangeText }}</p>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" md="6" lg="4">
          <v-card class="summary-card" rounded="xl" elevation="0">
            <v-card-text>
              <p class="summary-label">Maximum</p>
              <p class="summary-value">{{ formatNumber(stats?.max, config.key, config.digits) }}</p>
              <p class="summary-meta">{{ summaryRangeText }}</p>
            </v-card-text>
          </v-card>
        </v-col>
      </v-row>

      <section class="chart-panel">
        <div class="chart-header">
          <div>
            <h3 class="table-title">{{ chartTitleWithRange }}</h3>
            <p class="chart-range-copy">{{ selectedRangeText }}</p>
            <p class="chart-range-meta">{{ sampleSummaryText }}</p>
            <p v-if="actualResultRangeText" class="chart-range-meta">{{ actualResultRangeText }}</p>
          </div>
        </div>

        <div class="chart-wrap">
          <svg class="analysis-chart" viewBox="0 0 920 360" preserveAspectRatio="none">
            <g v-for="tick in chart.yTicks" :key="`y-${tick.value}`">
              <line
                :x1="chart.padding.left"
                :x2="chart.width - chart.padding.right"
                :y1="tick.y"
                :y2="tick.y"
                class="grid-line"
              />
              <text
                :x="chart.padding.left - 12"
                :y="tick.y + 4"
                class="axis-label axis-label--y"
                text-anchor="end"
              >
                {{ tick.label }}
              </text>
            </g>

            <line
              :x1="chart.padding.left"
              :x2="chart.padding.left"
              :y1="chart.padding.top"
              :y2="chart.height - chart.padding.bottom"
              class="axis-line"
            />
            <line
              :x1="chart.padding.left"
              :x2="chart.width - chart.padding.right"
              :y1="chart.height - chart.padding.bottom"
              :y2="chart.height - chart.padding.bottom"
              class="axis-line"
            />

            <g v-for="tick in chart.xTicks" :key="`x-${tick.value}`">
              <line
                :x1="tick.x"
                :x2="tick.x"
                :y1="chart.height - chart.padding.bottom"
                :y2="chart.height - chart.padding.bottom + 6"
                class="axis-line"
              />
              <text
                :x="tick.x"
                :y="chart.height - chart.padding.bottom + 20"
                class="axis-label"
                text-anchor="middle"
              >
                <tspan :x="tick.x" dy="0">{{ tick.label }}</tspan>
                <tspan v-if="tick.secondaryLabel" :x="tick.x" dy="12">{{ tick.secondaryLabel }}</tspan>
              </text>
            </g>

            <text
              :x="chart.padding.left - 54"
              :y="chart.height / 2"
              class="axis-title"
              text-anchor="middle"
              :transform="`rotate(-90 ${chart.padding.left - 54} ${chart.height / 2})`"
            >
              {{ axisLabel }}
            </text>
            <text
              :x="chart.width / 2"
              :y="chart.height - 12"
              class="axis-title"
              text-anchor="middle"
            >
              Time
            </text>

            <polyline
              v-if="chart.points"
              :points="chart.points"
              class="series-line"
              :style="{ stroke: config.color }"
            />

            <g v-for="point in chart.labelPoints" :key="point.key">
              <circle :cx="point.x" :cy="point.y" r="5" class="series-point" :style="{ fill: config.color }" />
              <text :x="point.x" :y="point.labelY" class="point-label" text-anchor="middle">
                {{ point.valueLabel }}
              </text>
            </g>
          </svg>
        </div>
      </section>

      <section class="table-panel">
        <div class="table-header">
          <div>
            <p class="analysis-kicker">Range results</p>
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
import { computed, onMounted, onUnmounted, ref, watch } from 'vue'
import { useRoute } from 'vue-router'
import { useWeatherStore } from '@/store/weather'

const weatherStore = useWeatherStore()
const route = useRoute()
const startDate = ref('')
const endDate = ref('')
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
    digits: 1,
    tableTitle: 'Temperature samples',
    columnLabel: 'Temperature',
    chartTitle: 'Temperature trend',
    axisBaseLabel: 'Temperature',
    color: '#f4b73b',
  },
  humidity: {
    title: 'Humidity analysis window',
    description: 'Inspect how the relative humidity range is behaving over the recent capture window.',
    key: 'humidity',
    digits: 1,
    tableTitle: 'Humidity samples',
    columnLabel: 'Humidity',
    chartTitle: 'Humidity trend',
    axisBaseLabel: 'Humidity',
    color: '#4a9fff',
  },
  'heat-index': {
    title: 'Heat index analysis window',
    description: 'Summarize heat index movement across the latest stored readings.',
    key: 'heatIndex',
    digits: 1,
    tableTitle: 'Heat index samples',
    columnLabel: 'Heat Index',
    chartTitle: 'Heat index trend',
    axisBaseLabel: 'Heat Index',
    color: '#ff7a5a',
  },
  pressure: {
    title: 'Pressure analysis window',
    description: 'Summarize atmospheric pressure range and stability.',
    key: 'pressure',
    digits: 1,
    tableTitle: 'Pressure samples',
    columnLabel: 'Pressure',
    chartTitle: 'Pressure trend',
    axisBaseLabel: 'Pressure',
    color: '#4f7a50',
  },
  altitude: {
    title: 'Estimated altitude analysis window',
    description: 'Review the altitude estimate derived from pressure using the configured sea-level reference.',
    key: 'altitude',
    digits: 1,
    tableTitle: 'Estimated altitude samples',
    columnLabel: 'Estimated Altitude',
    chartTitle: 'Estimated altitude trend',
    axisBaseLabel: 'Altitude',
    color: '#8d63c7',
  },
  'soil-moisture': {
    title: 'Soil moisture analysis window',
    description: 'Track the calibrated soil moisture percentage across the recent readings.',
    key: 'soilMoisturePercent',
    digits: 0,
    tableTitle: 'Soil moisture percentage samples',
    columnLabel: 'Soil Moisture %',
    chartTitle: 'Soil moisture trend',
    axisBaseLabel: 'Soil Moisture',
    color: '#00a6a6',
  },
}

function average(values) {
  if (!values.length) {
    return null
  }

  return values.reduce((sum, value) => sum + value, 0) / values.length
}

function formatNumber(value, metricKey, digits = 1) {
  return weatherStore.formatValue(metricKey, value, digits)
}

function buildTicks(minValue, maxValue, count, digits) {
  const spread = maxValue - minValue || 1
  return Array.from({ length: count }, (_, index) => {
    const ratio = index / (count - 1 || 1)
    const value = maxValue - spread * ratio
    return {
      value,
      label: value.toFixed(digits),
    }
  })
}

function formatDateInput(date) {
  return date.toISOString().slice(0, 10)
}

function getDefaultDateRange() {
  const end = new Date()
  const start = new Date(end)
  start.setDate(end.getDate() - 6)

  return {
    start: formatDateInput(start),
    end: formatDateInput(end),
  }
}

function getDateRangeFilters() {
  const filters = {}

  if (startDate.value) {
    const start = new Date(`${startDate.value}T00:00:00`)
    filters.startTs = Math.floor(start.getTime() / 1000)
  }

  if (endDate.value) {
    const end = new Date(`${endDate.value}T23:59:59`)
    filters.endTs = Math.floor(end.getTime() / 1000)
  }

  return filters
}

function parseDateValue(value, endOfDay = false) {
  if (!value) {
    return null
  }

  return new Date(`${value}T${endOfDay ? '23:59:59' : '00:00:00'}`)
}

function formatRangeDate(value, endOfDay = false) {
  const date = parseDateValue(value, endOfDay)
  return date ? date.toLocaleDateString([], { year: 'numeric', month: 'short', day: 'numeric' }) : '--'
}

function formatTickLabel(timestamp, includeDate = false) {
  const date = new Date(timestamp * 1000)

  if (includeDate) {
    return {
      tickLabel: date.toLocaleDateString([], { month: 'short', day: 'numeric' }),
      secondaryLabel: date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
    }
  }

  return {
    tickLabel: date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
    secondaryLabel: '',
  }
}

const config = computed(() => sensorConfig[route.params.sensor] || sensorConfig.temperature)
const rangeStartDate = computed(() => parseDateValue(startDate.value, false))
const rangeEndDate = computed(() => parseDateValue(endDate.value, true))
const rangeSpansMultipleDays = computed(() => {
  if (!rangeStartDate.value || !rangeEndDate.value) {
    return false
  }

  return rangeStartDate.value.toDateString() !== rangeEndDate.value.toDateString()
})
const selectedRangeText = computed(() => {
  if (!startDate.value && !endDate.value) {
    return 'Showing all loaded samples.'
  }

  return `Selected range: ${formatRangeDate(startDate.value)} to ${formatRangeDate(endDate.value, true)}`
})
const summaryRangeText = computed(() => {
  if (actualResultRangeText.value) {
    return actualResultRangeText.value
  }

  return selectedRangeText.value
})
const sampleSummaryText = computed(() => {
  const count = weatherStore.analysisData?.count || 0
  return `${count} sample${count === 1 ? '' : 's'} loaded for this date range`
})
const analysisStatusText = computed(() => {
  if (weatherStore.analysisLoading) {
    return 'Loading MongoDB-backed analysis for the selected date range...'
  }

  if (weatherStore.analysisError) {
    return `Analysis request failed: ${weatherStore.analysisError}`
  }

  return 'No analysis results were returned for the selected date range.'
})
const actualResultRangeText = computed(() => {
  const startTimestamp = Number(weatherStore.analysisData?.startTimestamp)
  const endTimestamp = Number(weatherStore.analysisData?.endTimestamp)

  if (!Number.isFinite(startTimestamp) || !Number.isFinite(endTimestamp)) {
    return ''
  }

  return `Data found from ${new Date(startTimestamp * 1000).toLocaleString()} to ${new Date(endTimestamp * 1000).toLocaleString()}`
})
const chartTitleWithRange = computed(() => {
  if (!startDate.value || !endDate.value) {
    return config.value.chartTitle
  }

  return `${config.value.chartTitle} for ${formatRangeDate(startDate.value)} to ${formatRangeDate(endDate.value, true)}`
})
const axisLabel = computed(() => {
  const unitLabel = weatherStore.getUnitLabel(config.value.key)
  return unitLabel ? `${config.value.axisBaseLabel} (${unitLabel})` : config.value.axisBaseLabel
})
const analysisStats = computed(() => weatherStore.analysisData?.stats?.[config.value.key] || null)

const stats = computed(() => {
  if (!analysisStats.value) {
    return null
  }

  return {
    min: weatherStore.convertValue(config.value.key, analysisStats.value.min),
    max: weatherStore.convertValue(config.value.key, analysisStats.value.max),
    avg: weatherStore.convertValue(config.value.key, analysisStats.value.avg),
  }
})

const recentRows = computed(() =>
  (weatherStore.analysisData?.recentRows || [])
    .map((reading) => ({
      timestamp: reading.timestamp,
      time: new Date(reading.timestamp * 1000).toLocaleString([], {
        year: rangeSpansMultipleDays.value ? 'numeric' : undefined,
        month: rangeSpansMultipleDays.value ? 'short' : undefined,
        day: rangeSpansMultipleDays.value ? 'numeric' : undefined,
        hour: '2-digit',
        minute: '2-digit',
      }),
      value: formatNumber(Number(reading[config.value.key]), config.value.key, config.value.digits),
    })),
)

const chart = computed(() => {
  const width = 920
  const height = 360
  const padding = {
    top: 24,
    right: 24,
    bottom: 54,
    left: 84,
  }

  const sourceReadings = (weatherStore.analysisData?.sampledReadings || [])
    .map((reading) => ({
      timestamp: Number(reading.timestamp),
      value: weatherStore.convertValue(config.value.key, Number(reading[config.value.key])),
    }))
    .filter((reading) => Number.isFinite(reading.timestamp) && Number.isFinite(reading.value))

  if (!sourceReadings.length) {
    return {
      width,
      height,
      padding,
      points: '',
      pointsList: [],
      yTicks: [],
      xTicks: [],
    }
  }

  const values = sourceReadings.map((reading) => reading.value)
  const minValue = Math.min(...values)
  const maxValue = Math.max(...values)
  const paddedMin = minValue === maxValue ? minValue - 1 : minValue - (maxValue - minValue) * 0.08
  const paddedMax = minValue === maxValue ? maxValue + 1 : maxValue + (maxValue - minValue) * 0.08
  const valueSpread = paddedMax - paddedMin || 1
  const plotWidth = width - padding.left - padding.right
  const plotHeight = height - padding.top - padding.bottom
  const stepX = sourceReadings.length > 1 ? plotWidth / (sourceReadings.length - 1) : 0

  const pointsList = sourceReadings.map((reading, index) => {
    const x = padding.left + stepX * index
    const normalized = (reading.value - paddedMin) / valueSpread
    const y = height - padding.bottom - normalized * plotHeight
    return {
      key: `${reading.timestamp}-${index}`,
      x: Number(x.toFixed(1)),
      y: Number(y.toFixed(1)),
      valueLabel: reading.value.toFixed(config.value.digits),
      ...formatTickLabel(reading.timestamp, rangeSpansMultipleDays.value),
    }
  })

  const labelIndexes = new Set()

  if (pointsList.length <= 8) {
    pointsList.forEach((_, index) => labelIndexes.add(index))
  } else {
    labelIndexes.add(0)
    labelIndexes.add(pointsList.length - 1)

    let minIndex = 0
    let maxIndex = 0
    values.forEach((value, index) => {
      if (value < values[minIndex]) {
        minIndex = index
      }

      if (value > values[maxIndex]) {
        maxIndex = index
      }
    })

    labelIndexes.add(minIndex)
    labelIndexes.add(maxIndex)
  }

  const labelPoints = [...labelIndexes]
    .sort((left, right) => left - right)
    .map((index, order) => {
      const point = pointsList[index]
      const labelWouldClipTop = point.y <= padding.top + 18
      const labelY = labelWouldClipTop ? point.y + 18 : point.y - (10 + (order % 2) * 10)

      return {
        ...point,
        labelY,
      }
    })

  const xTickIndexes = sourceReadings.length <= 5
    ? sourceReadings.map((_, index) => index)
    : [0, Math.floor((sourceReadings.length - 1) / 3), Math.floor(((sourceReadings.length - 1) * 2) / 3), sourceReadings.length - 1]

  const yTicks = buildTicks(paddedMin, paddedMax, 5, config.value.digits).map((tick) => {
    const normalized = (tick.value - paddedMin) / valueSpread
    const y = height - padding.bottom - normalized * plotHeight
    return {
      ...tick,
      y: Number(y.toFixed(1)),
    }
  })

  const xTicks = [...new Set(xTickIndexes)].map((index) => ({
    value: index,
    x: pointsList[index].x,
    label: pointsList[index].tickLabel,
    secondaryLabel: pointsList[index].secondaryLabel,
  }))

  return {
    width,
    height,
    padding,
    points: pointsList.map((point) => `${point.x},${point.y}`).join(' '),
    pointsList,
    labelPoints,
    yTicks,
    xTicks,
  }
})

onMounted(() => {
  const defaults = getDefaultDateRange()
  startDate.value = defaults.start
  endDate.value = defaults.end
  weatherStore.fetchWeatherAnalysis(getDateRangeFilters(), { sampleLimit: 240, tableLimit: 10 })
  pollTimer = window.setInterval(
    () => weatherStore.fetchWeatherAnalysis(getDateRangeFilters(), { sampleLimit: 240, tableLimit: 10 }),
    5000,
  )
})

onUnmounted(() => {
  window.clearInterval(pollTimer)
})

watch(
  () => route.params.sensor,
  () => {
    weatherStore.fetchWeatherAnalysis(getDateRangeFilters(), { sampleLimit: 240, tableLimit: 10 })
  },
)

async function applyDateRange() {
  await weatherStore.fetchWeatherAnalysis(getDateRangeFilters(), { sampleLimit: 240, tableLimit: 10 })
}

async function resetDateRange() {
  const defaults = getDefaultDateRange()
  startDate.value = defaults.start
  endDate.value = defaults.end
  await weatherStore.fetchWeatherAnalysis(getDateRangeFilters(), { sampleLimit: 240, tableLimit: 10 })
}
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
.filter-panel,
.chart-panel,
.table-panel,
.summary-card {
  border: 1px solid rgba(23, 36, 55, 0.08);
  border-radius: 28px;
  background: rgba(255, 255, 255, 0.8);
  box-shadow: 0 12px 28px rgba(34, 55, 80, 0.08);
}

.analysis-hero,
.filter-panel,
.chart-panel,
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

.analysis-status-panel {
  margin-bottom: 20px;
  padding: 16px 20px;
  border: 1px solid rgba(23, 36, 55, 0.08);
  border-radius: 20px;
  background: rgba(255, 255, 255, 0.82);
}

.analysis-status {
  margin: 0;
  color: #536981;
  font-weight: 700;
}

.analysis-status--error {
  color: #a12626;
}

.filter-panel {
  display: grid;
  grid-template-columns: 1.1fr 1.4fr;
  gap: 20px;
  margin-bottom: 20px;
}

.filter-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
  align-items: end;
}

.filter-field {
  display: grid;
  gap: 8px;
  color: #536981;
  font-weight: 700;
}

.date-input {
  padding: 12px 14px;
  border: 1px solid rgba(23, 36, 55, 0.12);
  border-radius: 14px;
  color: #172437;
  background: rgba(255, 255, 255, 0.85);
}

.filter-actions {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

.filter-button {
  min-height: 44px;
  padding: 0 16px;
  border: 1px solid rgba(36, 77, 115, 0.14);
  border-radius: 999px;
  color: #244d73;
  background: rgba(36, 77, 115, 0.08);
  font-weight: 700;
  cursor: pointer;
}

.filter-button--primary {
  border-color: #244d73;
  color: #f4f8fc;
  background: #244d73;
}

.chart-panel {
  margin-bottom: 20px;
}

.chart-header {
  margin-bottom: 16px;
}

.chart-wrap {
  overflow-x: auto;
}

.analysis-chart {
  width: 100%;
  height: 360px;
  border-radius: 18px;
  background:
    linear-gradient(to bottom, rgba(47, 72, 98, 0.05), rgba(47, 72, 98, 0.02)),
    linear-gradient(180deg, rgba(255, 255, 255, 0.65), rgba(232, 238, 245, 0.55));
}

.grid-line {
  stroke: rgba(23, 36, 55, 0.1);
  stroke-width: 1;
}

.axis-line {
  stroke: rgba(23, 36, 55, 0.35);
  stroke-width: 1.25;
}

.axis-label {
  fill: #6d7f92;
  font-size: 11px;
  font-weight: 700;
}

.axis-label--y {
  letter-spacing: 0.04em;
}

.axis-title {
  fill: #536981;
  font-size: 12px;
  font-weight: 800;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.series-line {
  fill: none;
  stroke-width: 4;
  stroke-linecap: round;
  stroke-linejoin: round;
}

.series-point {
  stroke: rgba(255, 255, 255, 0.9);
  stroke-width: 2;
}

.point-label {
  fill: #172437;
  font-size: 11px;
  font-weight: 700;
  paint-order: stroke;
  stroke: rgba(255, 255, 255, 0.92);
  stroke-width: 3px;
  stroke-linejoin: round;
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

.summary-meta,
.chart-range-copy,
.chart-range-meta {
  margin: 10px 0 0;
  color: #536981;
  line-height: 1.5;
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

@media (max-width: 960px) {
  .filter-panel,
  .filter-grid {
    grid-template-columns: 1fr;
  }
}
</style>
