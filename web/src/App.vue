<script setup lang="ts">
import { RouterLink, RouterView } from "vue-router";
</script>

<template>
  <div class="app-shell">
    <nav class="app-nav" aria-label="Development views">
      <RouterLink to="/">Player Chase</RouterLink>
      <RouterLink to="/dps">DPS</RouterLink>
    </nav>
    <RouterView />
  </div>
</template>

<style scoped>
.app-shell {
    min-height: 100vh;
}

.app-nav {
    align-items: center;
    background: #1e252b;
    display: flex;
    gap: 8px;
    padding: 10px 28px;
}

.app-nav a {
    border-radius: 6px;
    color: #dfe7eb;
    font-size: 0.9rem;
    font-weight: 800;
    padding: 8px 12px;
    text-decoration: none;
}

.app-nav a.router-link-active {
    background: #ffffff;
    color: #1e252b;
}
</style>
