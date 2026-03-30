import { defineStore } from 'pinia'

const configuredApiBaseUrl = import.meta.env.VITE_API_BASE_URL?.trim()
const defaultApiBaseUrl = `${window.location.protocol}//${window.location.hostname}:5000`
const API_BASE_URL = configuredApiBaseUrl || defaultApiBaseUrl
const SOIL_RAW_DRY = 720
const SOIL_RAW_WET = 160
const UNIT_SYSTEMS = {
  metric: 'metric',
  imperial: 'imperial',
}

function isMissingValue(value) {
  return value === null || value === undefined || value === ''
}

function formatTimestamp(timestamp) {
  if (!timestamp) {
    return 'No data yet'
  }

  return new Date(timestamp * 1000).toLocaleString()
}

function formatMetric(value, unit, digits = 1) {
  if (isMissingValue(value)) {
    return '--'
  }

  const numericValue = Number(value)

  if (!Number.isFinite(numericValue)) {
    return '--'
  }

  return `${numericValue.toFixed(digits)} ${unit}`.trim()
}

function celsiusToFahrenheit(value) {
  return (value * 9) / 5 + 32
}

function hpaToInHg(value) {
  return value * 0.0295299831
}

function metersToFeet(value) {
  return value * 3.28084
}

function convertMetricValue(metricKey, value, unitSystem) {
  if (isMissingValue(value)) {
    return NaN
  }

  const numericValue = Number(value)

  if (!Number.isFinite(numericValue)) {
    return NaN
  }

  if (unitSystem === UNIT_SYSTEMS.imperial) {
    switch (metricKey) {
      case 'temperature':
      case 'heatIndex':
        return celsiusToFahrenheit(numericValue)
      case 'pressure':
      case 'seaLevelPressureHpa':
        return hpaToInHg(numericValue)
      case 'altitude':
        return metersToFeet(numericValue)
      default:
        return numericValue
    }
  }

  return numericValue
}

function getMetricUnit(metricKey, unitSystem) {
  if (unitSystem === UNIT_SYSTEMS.imperial) {
    switch (metricKey) {
      case 'temperature':
      case 'heatIndex':
        return 'F'
      case 'pressure':
      case 'seaLevelPressureHpa':
        return 'inHg'
      case 'altitude':
        return 'ft'
      case 'humidity':
      case 'soilMoisturePercent':
        return '%'
      case 'soilRaw':
        return 'raw'
      default:
        return ''
    }
  }

  switch (metricKey) {
    case 'temperature':
    case 'heatIndex':
      return 'C'
    case 'pressure':
    case 'seaLevelPressureHpa':
      return 'hPa'
    case 'altitude':
      return 'm'
    case 'humidity':
    case 'soilMoisturePercent':
      return '%'
    case 'soilRaw':
      return 'raw'
    default:
      return ''
  }
}

function formatControlTimestamp(timestamp) {
  const numericTimestamp = Number(timestamp)

  if (!Number.isFinite(numericTimestamp) || numericTimestamp <= 0) {
    return '--'
  }

  return new Date(numericTimestamp * 1000).toLocaleString()
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

function createEmptyAnalysisData() {
  return {
    count: 0,
    startTimestamp: null,
    endTimestamp: null,
    stats: {},
    sampledReadings: [],
    recentRows: [],
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
    controlStatus: {
      maintenanceMode: false,
      lastAcknowledgedAt: null,
      commandQueueDepth: 0,
      latestWeatherTimestamp: null,
      latestDeviceId: '',
      pendingCommands: [],
      recentCommands: [],
      deliveryMode: 'backend-queued',
      latestWeatherStatus: null,
    },
    controlLoading: false,
    controlActionLoading: false,
    controlError: '',
    controlMessage: '',
    systemInfo: null,
    systemLoading: false,
    systemError: '',
    analysisData: createEmptyAnalysisData(),
    analysisLoading: false,
    analysisError: '',
    unitSystem: UNIT_SYSTEMS.metric,
    apiLogs: [],
  }),
  actions: {
    pushApiLog(entry) {
      this.apiLogs = [
        {
          timestamp: Date.now(),
          ...entry,
        },
        ...this.apiLogs,
      ].slice(0, 30)
    },
    setUnitSystem(unitSystem) {
      this.unitSystem = unitSystem === UNIT_SYSTEMS.imperial ? UNIT_SYSTEMS.imperial : UNIT_SYSTEMS.metric
      this.pushApiLog({
        level: 'info',
        area: 'ui',
        message: `Unit system changed to ${this.unitSystem}`,
      })
    },
    formatValue(metricKey, value, digits = 1) {
      const convertedValue = convertMetricValue(metricKey, value, this.unitSystem)
      return formatMetric(convertedValue, getMetricUnit(metricKey, this.unitSystem), digits)
    },
    convertValue(metricKey, value) {
      return convertMetricValue(metricKey, value, this.unitSystem)
    },
    getUnitLabel(metricKey) {
      return getMetricUnit(metricKey, this.unitSystem)
    },
    setLatestReading(reading) {
      this.latestReading = {
        ...this.latestReading,
        ...reading,
      }
    },
    async fetchLatestReading() {
      this.loading = true
      this.error = ''
      this.pushApiLog({
        level: 'info',
        area: 'fetch',
        message: 'Requesting latest weather reading',
        endpoint: `${API_BASE_URL}/api/weather/latest`,
      })

      try {
        const response = await fetch(`${API_BASE_URL}/api/weather/latest`)
        const payload = await response.json()

        if (!response.ok) {
          throw new Error(payload.error || payload.data?.message || 'Failed to fetch weather data')
        }

        const reading = normalizeReading(payload.data || {})
        this.latestRaw = reading
        this.setLatestReading({
          temperature: this.formatValue('temperature', reading.temperature),
          humidity: this.formatValue('humidity', reading.humidity),
          heatIndex: this.formatValue('heatIndex', reading.heatIndex),
          pressure: this.formatValue('pressure', reading.pressure),
          altitude: this.formatValue('altitude', reading.altitude),
          soilMoisture: this.formatValue('soilMoisturePercent', reading.soilMoisturePercent, 0),
          soilRaw: this.formatValue('soilRaw', reading.soilRaw, 0),
          sensorHealth: [reading.dhtOk, reading.bmpOk, reading.soilOk].every(Boolean)
            ? 'All sensors OK'
            : 'Using fallback sensor data',
          timestamp: formatTimestamp(reading.timestamp),
        })
        this.pushApiLog({
          level: 'success',
          area: 'fetch',
          message: 'Latest weather reading received',
          endpoint: `${API_BASE_URL}/api/weather/latest`,
        })
      } catch (error) {
        this.error = error.message
        this.pushApiLog({
          level: 'error',
          area: 'fetch',
          message: error.message,
          endpoint: `${API_BASE_URL}/api/weather/latest`,
        })
      } finally {
        this.loading = false
      }
    },
    async fetchRecentReadings(limit = 24, filters = {}) {
      const query = new URLSearchParams({ limit: String(limit) })
      if (Number.isFinite(Number(filters.startTs))) {
        query.set('start_ts', String(filters.startTs))
      }
      if (Number.isFinite(Number(filters.endTs))) {
        query.set('end_ts', String(filters.endTs))
      }
      const endpoint = `${API_BASE_URL}/api/weather/recent?${query.toString()}`
      this.pushApiLog({
        level: 'info',
        area: 'fetch',
        message: `Requesting recent weather readings (limit=${limit})`,
        endpoint,
      })
      try {
        const response = await fetch(endpoint)
        const payload = await response.json()

        if (!response.ok) {
          throw new Error(payload.error || payload.data?.message || 'Failed to fetch history')
        }

        this.recentReadings = (payload.data || []).map((reading) => normalizeReading(reading))
        this.pushApiLog({
          level: 'success',
          area: 'fetch',
          message: `Recent weather readings received (${this.recentReadings.length})`,
          endpoint,
        })
      } catch (error) {
        this.error = error.message
        this.pushApiLog({
          level: 'error',
          area: 'fetch',
          message: error.message,
          endpoint,
        })
      }
    },
    setControlStatus(payload = {}) {
      this.controlStatus = {
        ...this.controlStatus,
        ...payload,
        pendingCommands: Array.isArray(payload.pendingCommands) ? payload.pendingCommands : this.controlStatus.pendingCommands,
        recentCommands: Array.isArray(payload.recentCommands) ? payload.recentCommands : this.controlStatus.recentCommands,
      }
    },
    async fetchControlStatus() {
      this.controlLoading = true
      this.controlError = ''
      this.pushApiLog({
        level: 'info',
        area: 'fetch',
        message: 'Requesting control status',
        endpoint: `${API_BASE_URL}/api/control/status`,
      })

      try {
        const response = await fetch(`${API_BASE_URL}/api/control/status`)
        const payload = await response.json()

        if (!response.ok) {
          throw new Error(payload.error || payload.data?.message || 'Failed to fetch control status')
        }

        this.setControlStatus(payload.data || {})
        this.pushApiLog({
          level: 'success',
          area: 'fetch',
          message: 'Control status received',
          endpoint: `${API_BASE_URL}/api/control/status`,
        })
      } catch (error) {
        this.controlError = error.message
        this.pushApiLog({
          level: 'error',
          area: 'fetch',
          message: error.message,
          endpoint: `${API_BASE_URL}/api/control/status`,
        })
      } finally {
        this.controlLoading = false
      }
    },
    async submitControlAction(action, note = '') {
      this.controlActionLoading = true
      this.controlError = ''
      this.controlMessage = ''
      this.pushApiLog({
        level: 'info',
        area: 'action',
        message: `Submitting control action: ${action}`,
        endpoint: `${API_BASE_URL}/api/control/action`,
      })

      try {
        const response = await fetch(`${API_BASE_URL}/api/control/action`, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify({ action, note }),
        })
        const payload = await response.json()

        if (!response.ok) {
          throw new Error(payload.error || payload.data?.message || 'Failed to submit control action')
        }

        this.controlMessage = payload.data?.message || 'Control action completed'
        this.setControlStatus(payload.data?.control || {})
        this.pushApiLog({
          level: 'success',
          area: 'action',
          message: this.controlMessage,
          endpoint: `${API_BASE_URL}/api/control/action`,
        })
      } catch (error) {
        this.controlError = error.message
        this.pushApiLog({
          level: 'error',
          area: 'action',
          message: error.message,
          endpoint: `${API_BASE_URL}/api/control/action`,
        })
      } finally {
        this.controlActionLoading = false
      }
    },
    async fetchSystemInfo() {
      this.systemLoading = true
      this.systemError = ''
      this.pushApiLog({
        level: 'info',
        area: 'fetch',
        message: 'Requesting system and database info',
        endpoint: `${API_BASE_URL}/api/system/info`,
      })

      try {
        const response = await fetch(`${API_BASE_URL}/api/system/info`)
        const payload = await response.json()

        if (!response.ok) {
          throw new Error(payload.error || payload.data?.message || 'Failed to fetch system info')
        }

        this.systemInfo = payload.data || null
        this.pushApiLog({
          level: 'success',
          area: 'fetch',
          message: 'System and database info received',
          endpoint: `${API_BASE_URL}/api/system/info`,
        })
      } catch (error) {
        this.systemError = error.message
        this.pushApiLog({
          level: 'error',
          area: 'fetch',
          message: error.message,
          endpoint: `${API_BASE_URL}/api/system/info`,
        })
      } finally {
        this.systemLoading = false
      }
    },
    async fetchWeatherAnalysis(filters = {}, options = {}) {
      const query = new URLSearchParams()
      if (Number.isFinite(Number(filters.startTs))) {
        query.set('start_ts', String(filters.startTs))
      }
      if (Number.isFinite(Number(filters.endTs))) {
        query.set('end_ts', String(filters.endTs))
      }
      query.set('sample_limit', String(options.sampleLimit || 240))
      query.set('table_limit', String(options.tableLimit || 10))
      const endpoint = `${API_BASE_URL}/api/weather/analysis?${query.toString()}`

      this.analysisLoading = true
      this.analysisError = ''
      this.pushApiLog({
        level: 'info',
        area: 'fetch',
        message: 'Requesting weather analysis for selected date range',
        endpoint,
      })

      try {
        const response = await fetch(endpoint)
        const payload = await response.json()

        if (!response.ok) {
          throw new Error(payload.error || payload.data?.message || 'Failed to fetch weather analysis')
        }

        this.analysisData = {
          count: Number(payload.data?.count) || 0,
          startTimestamp: Number(payload.data?.startTimestamp) || null,
          endTimestamp: Number(payload.data?.endTimestamp) || null,
          stats: payload.data?.stats || {},
          sampledReadings: Array.isArray(payload.data?.sampledReadings)
            ? payload.data.sampledReadings.map((reading) => normalizeReading(reading))
            : [],
          recentRows: Array.isArray(payload.data?.recentRows)
            ? payload.data.recentRows.map((reading) => normalizeReading(reading))
            : [],
        }

        this.pushApiLog({
          level: 'success',
          area: 'fetch',
          message: `Weather analysis received (${this.analysisData.count} samples in range)`,
          endpoint,
        })
      } catch (error) {
        this.analysisError = error.message
        this.analysisData = createEmptyAnalysisData()
        this.pushApiLog({
          level: 'error',
          area: 'fetch',
          message: error.message,
          endpoint,
        })
      } finally {
        this.analysisLoading = false
      }
    },
    async refreshWeather(limit = 24, filters = {}) {
      await Promise.all([this.fetchLatestReading(), this.fetchRecentReadings(limit, filters)])
    },
    async refreshControl() {
      await Promise.all([this.fetchControlStatus(), this.fetchSystemInfo()])
    },
    formatControlTimestamp,
  },
})
