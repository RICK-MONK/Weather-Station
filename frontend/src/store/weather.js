import { defineStore } from 'pinia'

const configuredApiBaseUrl = import.meta.env.VITE_API_BASE_URL?.trim()
const defaultApiBaseUrl = `${window.location.protocol}//${window.location.hostname}:5000`
const API_BASE_URL = configuredApiBaseUrl || defaultApiBaseUrl
const SOIL_RAW_DRY = 720
const SOIL_RAW_WET = 160

function formatTimestamp(timestamp) {
  if (!timestamp) {
    return 'No data yet'
  }

  return new Date(timestamp * 1000).toLocaleString()
}

function formatMetric(value, unit, digits = 1) {
  const numericValue = Number(value)

  if (!Number.isFinite(numericValue)) {
    return '--'
  }

  return `${numericValue.toFixed(digits)} ${unit}`.trim()
}

function computeSoilPercent(rawValue) {
  const numericValue = Number(rawValue)

  if (!Number.isFinite(numericValue)) {
    return NaN
  }

  const percent = ((numericValue - SOIL_RAW_DRY) * 100) / (SOIL_RAW_WET - SOIL_RAW_DRY)
  return Math.min(100, Math.max(0, Math.round(percent)))
}

function normalizeSoilReading(reading = {}) {
  const explicitRaw = Number(reading.soilRaw)
  const explicitPercent = Number(reading.soilMoisturePercent)
  const legacySoilValue = Number(reading.soilMoisture)

  const soilRaw = Number.isFinite(explicitRaw)
    ? explicitRaw
    : Number.isFinite(explicitPercent)
      ? NaN
      : legacySoilValue

  const soilMoisturePercent = Number.isFinite(explicitPercent)
    ? explicitPercent
    : Number.isFinite(soilRaw)
      ? computeSoilPercent(soilRaw)
      : legacySoilValue

  return {
    soilRaw,
    soilMoisturePercent,
    soilMoisture: soilMoisturePercent,
  }
}

function normalizeReading(reading = {}) {
  const soilReading = normalizeSoilReading(reading)

  return {
    temperature: Number(reading.temperature),
    humidity: Number(reading.humidity),
    heatIndex: Number(reading.heatIndex),
    pressure: Number(reading.pressure),
    altitude: Number(reading.altitude),
    seaLevelPressureHpa: Number(reading.seaLevelPressureHpa),
    altitudeEstimated: reading.altitudeEstimated === undefined ? true : Boolean(Number(reading.altitudeEstimated)),
    soilMoisture: soilReading.soilMoisture,
    soilRaw: soilReading.soilRaw,
    soilMoisturePercent: soilReading.soilMoisturePercent,
    dhtOk: reading.dhtOk === undefined ? true : Boolean(Number(reading.dhtOk)),
    bmpOk: reading.bmpOk === undefined ? true : Boolean(Number(reading.bmpOk)),
    soilOk: reading.soilOk === undefined ? true : Boolean(Number(reading.soilOk)),
    sampleMillis: Number(reading.sampleMillis),
    timestamp: Number(reading.timestamp),
  }
}

export const useWeatherStore = defineStore('weather', {
  state: () => ({
    latestRaw: null,
    latestReading: {
      temperature: '--',
      humidity: '--',
      heatIndex: '--',
      pressure: '--',
      altitude: '--',
      soilMoisture: '--',
      soilRaw: '--',
      sensorHealth: 'All sensors OK',
      timestamp: 'No data yet',
    },
    recentReadings: [],
    loading: false,
    error: '',
  }),
  actions: {
    setLatestReading(reading) {
      this.latestReading = {
        ...this.latestReading,
        ...reading,
      }
    },
    async fetchLatestReading() {
      this.loading = true
      this.error = ''

      try {
        const response = await fetch(`${API_BASE_URL}/api/weather/latest`)
        const payload = await response.json()

        if (!response.ok) {
          throw new Error(payload.error || payload.data?.message || 'Failed to fetch weather data')
        }

        const reading = normalizeReading(payload.data || {})
        this.latestRaw = reading
        this.setLatestReading({
          temperature: formatMetric(reading.temperature, 'C'),
          humidity: formatMetric(reading.humidity, '%'),
          heatIndex: formatMetric(reading.heatIndex, 'C'),
          pressure: formatMetric(reading.pressure, 'hPa'),
          altitude: formatMetric(reading.altitude, 'm'),
          soilMoisture: formatMetric(reading.soilMoisturePercent, '%', 0),
          soilRaw: formatMetric(reading.soilRaw, 'raw', 0),
          sensorHealth: [reading.dhtOk, reading.bmpOk, reading.soilOk].every(Boolean)
            ? 'All sensors OK'
            : 'Using fallback sensor data',
          timestamp: formatTimestamp(reading.timestamp),
        })
      } catch (error) {
        this.error = error.message
      } finally {
        this.loading = false
      }
    },
    async fetchRecentReadings(limit = 24) {
      try {
        const response = await fetch(`${API_BASE_URL}/api/weather/recent?limit=${limit}`)
        const payload = await response.json()

        if (!response.ok) {
          throw new Error(payload.error || payload.data?.message || 'Failed to fetch history')
        }

        this.recentReadings = (payload.data || []).map((reading) => normalizeReading(reading))
      } catch (error) {
        this.error = error.message
      }
    },
    async refreshWeather(limit = 24) {
      await Promise.all([this.fetchLatestReading(), this.fetchRecentReadings(limit)])
    },
  },
})
