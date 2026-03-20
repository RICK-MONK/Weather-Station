# Frontend

Minimal Vue/Vuetify frontend scaffold for the weather station dashboard.

## Setup

```bash
npm install
npm run dev
```

The Vite dev server listens on `0.0.0.0` so the dashboard can be opened from another device on the same LAN.
By default the frontend calls `http://<current-host>:5000`; set `VITE_API_BASE_URL` if your API is hosted elsewhere.

## Notes

- `src/views/WeatherDashboard.vue` contains the starter dashboard page.
- `src/router/index.js` defines the dashboard route.
- `src/store/weather.js` contains a simple weather data store.
