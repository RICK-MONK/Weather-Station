import { defineStore } from 'pinia'

const API_BASE_URL = import.meta.env.VITE_API_BASE_URL || 'http://127.0.0.1:5000'

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

function normalizeReading(reading = {}) {
  return {
    temperature: Number(reading.temperature),
    humidity: Number(reading.humidity),
    heatIndex: Number(reading.heatIndex),
    pressure: Number(reading.pressure),
    altitude: Number(reading.altitude),
    soilMoisture: Number(reading.soilMoisture),
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
          soilMoisture: formatMetric(reading.soilMoisture, 'raw', 0),
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
