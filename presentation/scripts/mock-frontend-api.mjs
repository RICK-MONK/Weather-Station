import http from 'node:http'

const port = Number(process.env.PORT || 5000)
const now = Math.floor(Date.now() / 1000)

function makeReading(index) {
  const timestamp = now - (23 - index) * 300
  const temperature = 26.4 + Math.sin(index / 3) * 1.8
  const humidity = 62 + Math.cos(index / 4) * 8
  const heatIndex = temperature + 1.3
  const pressure = 1011.5 + Math.sin(index / 5) * 3.2
  const altitude = 32 + Math.cos(index / 6) * 2.4
  const soilRaw = 360 + Math.sin(index / 2.5) * 90
  const soilMoisturePercent = Math.max(0, Math.min(100, Math.round(((720 - soilRaw) / (720 - 160)) * 100)))

  return {
    temperature: Number(temperature.toFixed(1)),
    humidity: Number(humidity.toFixed(1)),
    heatIndex: Number(heatIndex.toFixed(1)),
    pressure: Number(pressure.toFixed(1)),
    altitude: Number(altitude.toFixed(1)),
    seaLevelPressureHpa: 1013.25,
    altitudeEstimated: 1,
    soilRaw: Math.round(soilRaw),
    soilMoisturePercent,
    dhtOk: 1,
    bmpOk: 1,
    soilOk: 1,
    sampleMillis: index * 2000,
    timestamp,
  }
}

const recentReadings = Array.from({ length: 24 }, (_, index) => makeReading(index))
const latestReading = recentReadings[recentReadings.length - 1]

function sendJson(response, statusCode, data) {
  response.writeHead(statusCode, {
    'Content-Type': 'application/json',
    'Access-Control-Allow-Origin': '*',
    'Access-Control-Allow-Methods': 'GET,POST,OPTIONS',
    'Access-Control-Allow-Headers': 'Content-Type',
  })
  response.end(JSON.stringify(data))
}

function buildAnalysis(metricKey) {
  const values = recentReadings.map((reading) => Number(reading[metricKey])).filter(Number.isFinite)
  const min = Math.min(...values)
  const max = Math.max(...values)
  const avg = values.reduce((sum, value) => sum + value, 0) / values.length

  return {
    count: recentReadings.length,
    startTimestamp: recentReadings[0].timestamp,
    endTimestamp: recentReadings[recentReadings.length - 1].timestamp,
    stats: {
      temperature: metricKey === 'temperature' ? { min, avg, max } : summary('temperature'),
      humidity: metricKey === 'humidity' ? { min, avg, max } : summary('humidity'),
      heatIndex: metricKey === 'heatIndex' ? { min, avg, max } : summary('heatIndex'),
      pressure: metricKey === 'pressure' ? { min, avg, max } : summary('pressure'),
      altitude: metricKey === 'altitude' ? { min, avg, max } : summary('altitude'),
      soilMoisturePercent: metricKey === 'soilMoisturePercent' ? { min, avg, max } : summary('soilMoisturePercent'),
    },
    sampledReadings: recentReadings,
    recentRows: recentReadings.slice(-10).reverse(),
  }
}

function summary(metricKey) {
  const values = recentReadings.map((reading) => Number(reading[metricKey])).filter(Number.isFinite)
  return {
    min: Math.min(...values),
    avg: values.reduce((sum, value) => sum + value, 0) / values.length,
    max: Math.max(...values),
  }
}

const server = http.createServer(async (request, response) => {
  const url = new URL(request.url || '/', `http://${request.headers.host}`)

  if (request.method === 'OPTIONS') {
    sendJson(response, 204, {})
    return
  }

  if (request.method === 'GET' && url.pathname === '/api/weather/latest') {
    sendJson(response, 200, { data: latestReading })
    return
  }

  if (request.method === 'GET' && url.pathname === '/api/weather/recent') {
    sendJson(response, 200, { data: recentReadings })
    return
  }

  if (request.method === 'GET' && url.pathname === '/api/weather/analysis') {
    const sensorMap = {
      temperature: 'temperature',
      humidity: 'humidity',
      'heat-index': 'heatIndex',
      pressure: 'pressure',
      altitude: 'altitude',
      'soil-moisture': 'soilMoisturePercent',
    }
    const requested = url.searchParams.get('sensor')
    const metricKey = sensorMap[requested] || 'temperature'
    sendJson(response, 200, { data: buildAnalysis(metricKey) })
    return
  }

  if (request.method === 'GET' && url.pathname === '/api/control/status') {
    sendJson(response, 200, {
      data: {
        maintenanceMode: false,
        lastAcknowledgedAt: now - 900,
        commandQueueDepth: 2,
        latestWeatherTimestamp: latestReading.timestamp,
        latestDeviceId: 'gateway-esp32',
        deliveryMode: 'backend-queued',
        latestWeatherStatus: {
          status: 'healthy',
          message: 'Latest sensor packet accepted by backend',
        },
        pendingCommands: [
          {
            id: 'cmd-1',
            command: 'refresh-now',
            target: 'gateway',
            status: 'queued',
            createdAt: now - 240,
          },
          {
            id: 'cmd-2',
            command: 'restart',
            target: 'sensor',
            status: 'queued',
            createdAt: now - 120,
          },
        ],
        recentCommands: [
          {
            id: 'cmd-3',
            command: 'acknowledge-alerts',
            target: 'dashboard',
            status: 'applied',
            createdAt: now - 1800,
          },
          {
            id: 'cmd-4',
            command: 'clear-pending-commands',
            target: 'backend',
            status: 'cleared',
            createdAt: now - 5400,
          },
        ],
      },
    })
    return
  }

  if (request.method === 'GET' && url.pathname === '/api/system/info') {
    sendJson(response, 200, {
      data: {
        database: {
          engine: 'MongoDB',
          database: 'weather_station',
          collection: 'weather_readings',
          host: 'localhost',
          port: 27017,
          uriConfigured: true,
        },
        schema: {
          fields: [
            { name: 'temperature', type: 'number' },
            { name: 'humidity', type: 'number' },
            { name: 'heatIndex', type: 'number' },
            { name: 'pressure', type: 'number' },
            { name: 'altitude', type: 'number' },
            { name: 'soilMoisturePercent', type: 'number' },
            { name: 'timestamp', type: 'number' },
          ],
        },
      },
    })
    return
  }

  if (request.method === 'POST' && url.pathname === '/api/control/action') {
    sendJson(response, 200, {
      data: {
        message: 'Mock control action accepted',
        control: {
          maintenanceMode: false,
          commandQueueDepth: 2,
        },
      },
    })
    return
  }

  sendJson(response, 404, { error: 'Not found' })
})

server.listen(port, '127.0.0.1', () => {
  console.log(`Mock frontend API listening on http://127.0.0.1:${port}`)
})
