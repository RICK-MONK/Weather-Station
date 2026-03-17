import { createRouter, createWebHistory } from 'vue-router'
import WeatherDashboard from '@/views/WeatherDashboard.vue'
import WeatherLive from '@/views/WeatherLive.vue'
import WeatherControl from '@/views/WeatherControl.vue'
import WeatherAnalysis from '@/views/WeatherAnalysis.vue'

const routes = [
  {
    path: '/',
    redirect: '/home',
  },
  {
    path: '/home',
    name: 'dashboard',
    component: WeatherDashboard,
  },
  {
    path: '/live',
    redirect: '/live/temperature',
  },
  {
    path: '/live/:sensor',
    name: 'live',
    component: WeatherLive,
  },
  {
    path: '/control',
    name: 'control',
    component: WeatherControl,
  },
  {
    path: '/analysis',
    redirect: '/analysis/temperature',
  },
  {
    path: '/analysis/:sensor',
    name: 'analysis',
    component: WeatherAnalysis,
  },
]

const router = createRouter({
  history: createWebHistory(),
  routes,
})

export default router
