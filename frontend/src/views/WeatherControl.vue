<template>
  <v-main class="control-shell">
    <v-container class="control-container">
      <section class="control-hero">
        <div>
          <p class="control-kicker">Control</p>
          <h2 class="control-title">Gateway and system operations.</h2>
          <p class="control-copy">
            Send backend-tracked control actions, inspect queued device commands, and watch whether the weather pipeline is healthy enough to trust the live dashboard.
          </p>
        </div>

        <div class="hero-card">
          <p class="hero-label">Delivery mode</p>
          <p class="hero-value">{{ weatherStore.controlStatus.deliveryMode }}</p>
          <p class="hero-meta">
            Commands are recorded by Flask immediately. Device-side command pickup can be added later without changing this UI contract.
          </p>
        </div>
      </section>

      <section v-if="bannerMessage" class="banner-row">
        <div class="banner" :class="bannerClass">
          {{ bannerMessage }}
        </div>
      </section>

      <v-row dense class="control-grid">
        <v-col cols="12" sm="6" lg="3">
          <v-card class="control-card" rounded="xl" elevation="0">
            <v-card-text>
              <p class="control-label">Gateway state</p>
              <p class="control-value">{{ gatewayState.text }}</p>
              <p class="control-caption">{{ gatewayState.caption }}</p>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" sm="6" lg="3">
          <v-card class="control-card" rounded="xl" elevation="0">
            <v-card-text>
              <div class="toggle-row">
                <div>
                  <p class="control-label">Maintenance mode</p>
                  <p class="control-value control-value--compact">{{ maintenanceModeText }}</p>
                </div>
                <v-switch
                  :model-value="weatherStore.controlStatus.maintenanceMode"
                  color="#244d73"
                  hide-details
                  inset
                  :disabled="weatherStore.controlActionLoading"
                  @update:model-value="handleMaintenanceToggle"
                />
              </div>
              <p class="control-caption">Use maintenance mode to mark the station as intentionally being serviced.</p>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" sm="6" lg="3">
          <v-card class="control-card" rounded="xl" elevation="0">
            <v-card-text>
              <p class="control-label">Queued commands</p>
              <p class="control-value">{{ weatherStore.controlStatus.commandQueueDepth }}</p>
              <p class="control-caption">Pending restart or refresh requests recorded by the backend.</p>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" sm="6" lg="3">
          <v-card class="control-card" rounded="xl" elevation="0">
            <v-card-text>
              <p class="control-label">Last backend reading</p>
              <p class="control-value control-value--compact">{{ lastReadingText }}</p>
              <p class="control-caption">Latest device: {{ weatherStore.controlStatus.latestDeviceId || '--' }}</p>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" sm="6" lg="3">
          <v-card class="control-card" rounded="xl" elevation="0">
            <v-card-text>
              <div class="toggle-row">
                <div>
                  <p class="control-label">Units</p>
                  <p class="control-value control-value--compact">{{ unitSystemLabel }}</p>
                </div>
                <v-btn-toggle
                  :model-value="weatherStore.unitSystem"
                  density="comfortable"
                  mandatory
                  divided
                  @update:model-value="updateUnitSystem"
                >
                  <v-btn value="metric">Metric</v-btn>
                  <v-btn value="imperial">Imperial</v-btn>
                </v-btn-toggle>
              </div>
              <p class="control-caption">Unit conversion happens in the frontend render layer.</p>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" lg="8">
          <v-card class="control-card action-card" rounded="xl" elevation="0">
            <v-card-text>
              <div class="section-head">
                <div>
                  <p class="control-label">Control actions</p>
                  <h3 class="section-title">Send commands and refresh the console.</h3>
                </div>
                <v-btn
                  variant="outlined"
                  class="refresh-button"
                  :loading="weatherStore.loading || weatherStore.controlLoading"
                  @click="refreshConsole"
                >
                  Refresh now
                </v-btn>
              </div>

              <div class="button-grid">
                <v-btn class="action-button" color="#244d73" :loading="isActionBusy('queue-gateway-refresh')" @click="submitAction('queue-gateway-refresh')">
                  Queue gateway refresh
                </v-btn>
                <v-btn class="action-button" color="#244d73" :loading="isActionBusy('queue-gateway-restart')" @click="submitAction('queue-gateway-restart')">
                  Queue gateway restart
                </v-btn>
                <v-btn class="action-button" color="#244d73" :loading="isActionBusy('queue-sensor-restart')" @click="submitAction('queue-sensor-restart')">
                  Queue sensor restart
                </v-btn>
                <v-btn class="action-button" variant="tonal" color="#6f7f8f" :loading="isActionBusy('acknowledge-alerts')" @click="submitAction('acknowledge-alerts')">
                  Acknowledge alerts
                </v-btn>
                <v-btn class="action-button" variant="tonal" color="#6f7f8f" :loading="isActionBusy('clear-pending-commands')" @click="submitAction('clear-pending-commands')">
                  Clear pending queue
                </v-btn>
              </div>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" lg="4">
          <v-card class="control-card" rounded="xl" elevation="0">
            <v-card-text>
              <p class="control-label">Operations summary</p>
              <ul class="summary-list">
                <li>Polling cadence: 5 seconds</li>
                <li>Auto refresh: enabled for cards, graphs, and tables</li>
                <li>Last acknowledgement: {{ lastAcknowledgedText }}</li>
                <li>Backend status: {{ latestWeatherStatusText }}</li>
                <li>Pending queue visibility is API-backed and shared across browser sessions.</li>
              </ul>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" md="6">
          <v-card class="control-card" rounded="xl" elevation="0">
            <v-card-text>
              <div class="section-head">
                <div>
                  <p class="control-label">Pending commands</p>
                  <h3 class="section-title">What the backend is holding for devices.</h3>
                </div>
                <span class="badge-pill">{{ pendingCommands.length }}</span>
              </div>

              <div v-if="pendingCommands.length" class="command-list">
                <article v-for="command in pendingCommands" :key="command.id" class="command-item">
                  <div class="command-topline">
                    <strong>{{ humanizeCommand(command.command) }}</strong>
                    <span class="status-pill status-pill--queued">{{ command.status }}</span>
                  </div>
                  <p class="command-meta">{{ command.target }} - {{ formatTimestamp(command.createdAt) }}</p>
                </article>
              </div>
              <p v-else class="empty-state">No pending device commands.</p>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" md="6">
          <v-card class="control-card" rounded="xl" elevation="0">
            <v-card-text>
              <div class="section-head">
                <div>
                  <p class="control-label">Recent activity</p>
                  <h3 class="section-title">Latest applied or queued actions.</h3>
                </div>
                <span class="badge-pill">{{ recentCommands.length }}</span>
              </div>

              <div v-if="recentCommands.length" class="command-list">
                <article v-for="command in recentCommands" :key="command.id" class="command-item">
                  <div class="command-topline">
                    <strong>{{ humanizeCommand(command.command) }}</strong>
                    <span class="status-pill" :class="statusClass(command.status)">{{ command.status }}</span>
                  </div>
                  <p class="command-meta">{{ command.target }} - {{ formatTimestamp(command.createdAt) }}</p>
                </article>
              </div>
              <p v-else class="empty-state">No control activity yet.</p>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" md="6">
          <v-card class="control-card" rounded="xl" elevation="0">
            <v-card-text>
              <div class="section-head">
                <div>
                  <p class="control-label">Database config</p>
                  <h3 class="section-title">MongoDB connection and storage layout.</h3>
                </div>
                <span class="badge-pill">{{ systemDatabase?.engine || '--' }}</span>
              </div>

              <ul class="summary-list summary-list--tight">
                <li>Database: {{ systemDatabase?.database || '--' }}</li>
                <li>Collection: {{ systemDatabase?.collection || '--' }}</li>
                <li>Host: {{ systemDatabase?.host || '--' }}:{{ systemDatabase?.port || '--' }}</li>
                <li>URI configured: {{ systemDatabase?.uriConfigured ? 'Yes' : 'No' }}</li>
              </ul>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" md="6">
          <v-card class="control-card" rounded="xl" elevation="0">
            <v-card-text>
              <div class="section-head">
                <div>
                  <p class="control-label">Schema design</p>
                  <h3 class="section-title">Stored weather document fields.</h3>
                </div>
                <span class="badge-pill">{{ schemaFields.length }}</span>
              </div>

              <div v-if="schemaFields.length" class="schema-grid">
                <div v-for="field in schemaFields" :key="field.name" class="schema-item">
                  <strong>{{ field.name }}</strong>
                  <span>{{ field.type }}</span>
                </div>
              </div>
              <p v-else class="empty-state">No schema metadata loaded.</p>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12">
          <v-card class="control-card" rounded="xl" elevation="0">
            <v-card-text>
              <div class="section-head">
                <div>
                  <p class="control-label">Console log</p>
                  <h3 class="section-title">Frontend Fetch API activity and rendering events.</h3>
                </div>
                <span class="badge-pill">{{ apiLogs.length }}</span>
              </div>

              <div v-if="apiLogs.length" class="log-list">
                <article v-for="(entry, index) in apiLogs" :key="`${entry.timestamp}-${index}`" class="log-item">
                  <div class="command-topline">
                    <strong>{{ entry.area }}</strong>
                    <span class="status-pill" :class="logClass(entry.level)">{{ entry.level }}</span>
                  </div>
                  <p class="command-meta">{{ formatLogTimestamp(entry.timestamp) }} - {{ entry.message }}</p>
                  <p v-if="entry.endpoint" class="log-endpoint">{{ entry.endpoint }}</p>
                </article>
              </div>
              <p v-else class="empty-state">No API log entries yet.</p>
            </v-card-text>
          </v-card>
        </v-col>
      </v-row>
    </v-container>
  </v-main>
</template>

<script setup>
import { computed, onMounted, onUnmounted, ref } from 'vue'
import { useWeatherStore } from '@/store/weather'

const weatherStore = useWeatherStore()
const activeAction = ref('')
let pollTimer

function formatAgeSeconds(timestamp) {
  const numericTimestamp = Number(timestamp)

  if (!Number.isFinite(numericTimestamp)) {
    return null
  }

  return Math.max(0, Math.floor(Date.now() / 1000 - numericTimestamp))
}

function humanizeCommand(command) {
  const map = {
    'refresh-now': 'Refresh gateway fetch',
    restart: 'Restart device',
    'acknowledge-alerts': 'Acknowledge alerts',
    'clear-pending-commands': 'Clear pending queue',
    'maintenance-mode': 'Maintenance mode',
  }

  return map[command] || command
}

function statusClass(status) {
  if (status === 'queued') {
    return 'status-pill--queued'
  }

  if (status === 'cleared') {
    return 'status-pill--cleared'
  }

  return 'status-pill--applied'
}

function logClass(level) {
  if (level === 'error') {
    return 'status-pill--cleared'
  }

  if (level === 'success') {
    return 'status-pill--applied'
  }

  return 'status-pill--queued'
}

const pendingCommands = computed(() => weatherStore.controlStatus.pendingCommands || [])
const recentCommands = computed(() => weatherStore.controlStatus.recentCommands || [])
const apiLogs = computed(() => weatherStore.apiLogs || [])
const systemDatabase = computed(() => weatherStore.systemInfo?.database || null)
const schemaFields = computed(() => weatherStore.systemInfo?.schema?.fields || [])
const unitSystemLabel = computed(() => weatherStore.unitSystem === 'imperial' ? 'Imperial' : 'Metric')

const gatewayState = computed(() => {
  if (weatherStore.error) {
    return {
      text: 'Backend error',
      caption: weatherStore.error,
    }
  }

  const timestamp = weatherStore.latestRaw?.timestamp
  const ageSeconds = formatAgeSeconds(timestamp)

  if (ageSeconds === null) {
    return {
      text: weatherStore.loading ? 'Syncing' : 'Waiting',
      caption: 'No weather reading has been loaded into the dashboard yet.',
    }
  }

  if (ageSeconds <= 10) {
    return {
      text: 'Healthy',
      caption: `Latest reading is ${ageSeconds}s old.`,
    }
  }

  if (ageSeconds <= 20) {
    return {
      text: 'Delayed',
      caption: `Latest reading is ${ageSeconds}s old.`,
    }
  }

  return {
    text: 'Stale',
    caption: `Latest reading is ${ageSeconds}s old.`,
  }
})

const maintenanceModeText = computed(() =>
  weatherStore.controlStatus.maintenanceMode ? 'Enabled' : 'Disabled',
)

const lastReadingText = computed(() =>
  weatherStore.formatControlTimestamp(weatherStore.controlStatus.latestWeatherTimestamp),
)

const lastAcknowledgedText = computed(() =>
  weatherStore.formatControlTimestamp(weatherStore.controlStatus.lastAcknowledgedAt),
)

const latestWeatherStatusText = computed(() => {
  const latestStatus = weatherStore.controlStatus.latestWeatherStatus
  return latestStatus?.message || latestStatus?.status || 'Unknown'
})

const bannerMessage = computed(() =>
  weatherStore.controlError || weatherStore.controlMessage || '',
)

const bannerClass = computed(() =>
  weatherStore.controlError ? 'banner--danger' : 'banner--info',
)

async function refreshConsole() {
  await Promise.all([weatherStore.refreshWeather(20), weatherStore.refreshControl()])
}

async function submitAction(action) {
  activeAction.value = action
  await weatherStore.submitControlAction(action)
  activeAction.value = ''
}

async function handleMaintenanceToggle(enabled) {
  await submitAction(enabled ? 'enable-maintenance' : 'disable-maintenance')
}

function updateUnitSystem(unitSystem) {
  weatherStore.setUnitSystem(unitSystem)
}

function isActionBusy(action) {
  return weatherStore.controlActionLoading && activeAction.value === action
}

function formatTimestamp(timestamp) {
  return weatherStore.formatControlTimestamp(timestamp)
}

function formatLogTimestamp(timestamp) {
  return new Date(timestamp).toLocaleTimeString()
}

onMounted(() => {
  refreshConsole()
  pollTimer = window.setInterval(() => {
    refreshConsole()
  }, 5000)
})

onUnmounted(() => {
  window.clearInterval(pollTimer)
})
</script>

<style scoped>
.control-shell {
  min-height: 100vh;
  background:
    radial-gradient(circle at top left, rgba(255, 214, 155, 0.3), transparent 28%),
    radial-gradient(circle at top right, rgba(117, 175, 221, 0.24), transparent 26%),
    linear-gradient(180deg, #f5efe5 0%, #e8eef5 100%);
}

.control-container {
  padding-top: 40px;
  padding-bottom: 40px;
}

.control-hero {
  display: grid;
  grid-template-columns: 1.7fr 1fr;
  gap: 20px;
  margin-bottom: 20px;
}

.control-kicker {
  margin: 0 0 8px;
  color: #8e5a23;
  font-size: 0.78rem;
  font-weight: 700;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.control-title {
  margin: 0;
  color: #172437;
  font-size: clamp(1.9rem, 4vw, 3rem);
  line-height: 1.04;
}

.control-copy {
  max-width: 60ch;
  margin: 16px 0 0;
  color: #536981;
  line-height: 1.6;
}

.hero-card,
.control-card {
  border: 1px solid rgba(23, 36, 55, 0.08);
  background: rgba(255, 255, 255, 0.78);
  box-shadow: 0 12px 28px rgba(34, 55, 80, 0.08);
}

.hero-card {
  padding: 24px;
  border-radius: 28px;
}

.hero-label,
.control-label {
  margin: 0 0 10px;
  color: #6d7f92;
  font-size: 0.78rem;
  font-weight: 700;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.hero-value,
.control-value {
  margin: 0;
  color: #172437;
  font-size: clamp(1.7rem, 3vw, 2.4rem);
  font-weight: 800;
  line-height: 1.04;
}

.control-value--compact {
  font-size: 1.3rem;
}

.hero-meta,
.control-caption,
.command-meta,
.empty-state {
  color: #566d83;
  line-height: 1.6;
}

.hero-meta,
.control-caption {
  margin: 12px 0 0;
}

.banner-row {
  margin-bottom: 16px;
}

.banner {
  padding: 14px 18px;
  border-radius: 18px;
  font-weight: 700;
}

.banner--info {
  background: #e7f0fa;
  color: #244d73;
}

.banner--danger {
  background: #fde3e3;
  color: #9b2c2c;
}

.control-grid {
  row-gap: 16px;
}

.toggle-row,
.section-head,
.command-topline {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.section-title {
  margin: 0;
  color: #172437;
  font-size: 1.15rem;
}

.action-card {
  height: 100%;
}

.refresh-button {
  border-color: rgba(36, 77, 115, 0.2);
  color: #244d73;
}

.button-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
  margin-top: 18px;
}

.action-button {
  min-height: 48px;
  font-weight: 700;
  text-transform: none;
}

.summary-list {
  margin: 0;
  padding-left: 18px;
  color: #4f6478;
  line-height: 1.8;
}

.summary-list--tight {
  margin-top: 16px;
}

.badge-pill,
.status-pill {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 2rem;
  padding: 0.28rem 0.7rem;
  border-radius: 999px;
  font-size: 0.75rem;
  font-weight: 700;
  line-height: 1;
}

.badge-pill,
.status-pill--applied {
  background: #e8edf3;
  color: #49637a;
}

.status-pill--queued {
  background: #fff0d6;
  color: #9a5b00;
}

.status-pill--cleared {
  background: #fde3e3;
  color: #a12626;
}

.command-list {
  display: grid;
  gap: 12px;
  margin-top: 18px;
}

.schema-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
  margin-top: 18px;
}

.schema-item,
.log-item {
  padding: 14px 16px;
  border-radius: 18px;
  background: rgba(243, 247, 251, 0.9);
}

.command-item {
  padding: 14px 16px;
  border-radius: 18px;
  background: rgba(243, 247, 251, 0.9);
}

.command-meta,
.empty-state {
  margin: 8px 0 0;
}

.log-list {
  display: grid;
  gap: 12px;
  margin-top: 18px;
}

.log-endpoint {
  margin: 6px 0 0;
  color: #6d7f92;
  font-family: Consolas, 'Courier New', monospace;
  font-size: 0.82rem;
}

@media (max-width: 960px) {
  .control-hero {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 760px) {
  .button-grid {
    grid-template-columns: 1fr;
  }

  .schema-grid {
    grid-template-columns: 1fr;
  }

  .section-head,
  .toggle-row,
  .command-topline {
    align-items: flex-start;
    flex-direction: column;
  }
}
</style>
