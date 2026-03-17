<template>
  <v-app>
    <v-app-bar class="app-bar" flat>
      <div class="brand-block">
        <p class="brand-kicker">ELET2415 Project</p>
        <h1 class="brand-title">Weather Station Console</h1>
      </div>

      <div class="nav-center">
        <nav class="nav-tabs">
          <RouterLink v-for="tab in tabs" :key="tab.to" :to="tab.to" class="nav-tab" active-class="nav-tab--active">
            {{ tab.label }}
          </RouterLink>
        </nav>
      </div>
    </v-app-bar>

    <router-view />
  </v-app>
</template>

<script setup>
const tabs = [
  { label: 'Home', to: '/home' },
  { label: 'Live', to: '/live' },
  { label: 'Control', to: '/control' },
  { label: 'Analysis', to: '/analysis' },
]
</script>

<style scoped>
.app-bar {
  position: relative;
  padding: 0 24px;
  border-bottom: 1px solid rgba(23, 36, 55, 0.08);
  background: rgba(251, 247, 239, 0.9);
  backdrop-filter: blur(10px);
}

.brand-block {
  display: flex;
  flex-direction: column;
}

.brand-kicker {
  margin: 0;
  color: #9b5c22;
  font-size: 0.72rem;
  font-weight: 700;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.brand-title {
  margin: 2px 0 0;
  color: #172437;
  font-size: 1.05rem;
  font-weight: 800;
}

.nav-center {
  position: absolute;
  left: 50%;
  transform: translateX(-50%);
}

.nav-tabs {
  display: flex;
  gap: 14px;
}

.nav-tab {
  padding: 12px 20px;
  border-radius: 999px;
  color: #5c6f84;
  font-size: 1rem;
  font-weight: 700;
  text-decoration: none;
  transition: background-color 0.2s ease, color 0.2s ease;
}

.nav-tab:hover {
  background: rgba(36, 77, 115, 0.08);
}

.nav-tab--active {
  background: #244d73;
  color: #f4f8fc;
}

@media (max-width: 760px) {
  .app-bar {
    padding: 0 12px;
    min-height: 96px;
  }

  .nav-center {
    position: static;
    transform: none;
    margin-left: auto;
  }

  .nav-tab {
    padding: 8px 10px;
    font-size: 0.82rem;
  }
}
</style>
