<script setup lang="ts">
import { computed, onMounted, ref } from "vue";
import {
    calculateDpsScenarioResults,
    formatDpsNumber,
    type DpsScenarioResult,
} from "./dpsCalculator";
import { loadEngineModule } from "./wasm/EngineModule";

const engineModuleStatus = ref<"loading" | "loaded" | "failed">("loading");
const scenarioResults = ref<DpsScenarioResult[]>([]);

const engineStatusLabel = computed(() => {
    switch (engineModuleStatus.value) {
        case "loaded":
            return "Engine wasm loaded";
        case "failed":
            return "Engine wasm unavailable";
        case "loading":
            return "Loading engine wasm";
    }
});

onMounted(async () => {
    try {
        const module = await loadEngineModule();
        scenarioResults.value = calculateDpsScenarioResults(module);
        engineModuleStatus.value = "loaded";
    } catch (error) {
        console.error("Failed to load engine wasm module", error);
        engineModuleStatus.value = "failed";
    }
});
</script>

<template>
  <main class="dps-shell">
    <header class="dps-header">
      <div>
        <p class="eyebrow">Development viewer</p>
        <h1>DPS Calculator</h1>
        <p :class="['engine-status', engineModuleStatus]">
          {{ engineStatusLabel }}
        </p>
      </div>
    </header>

    <section v-if="scenarioResults.length > 0" class="dps-table-wrap">
      <table class="dps-table">
        <thead>
          <tr>
            <th>Scenario</th>
            <th>Type</th>
            <th>Attack Roll</th>
            <th>Defence Roll</th>
            <th>Hit Chance</th>
            <th>Maximum Hit</th>
            <th>Expected Damage</th>
            <th>Sample Seed</th>
            <th>Accuracy</th>
            <th>Sampled Damage</th>
            <th>Seconds</th>
            <th>DPS</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="entry in scenarioResults" :key="entry.scenario.name">
            <td>{{ entry.scenario.name }}</td>
            <td>{{ entry.scenario.attackTypeLabel }}</td>
            <td>{{ entry.result.attackRoll }}</td>
            <td>{{ entry.result.defenceRoll }}</td>
            <td>{{ formatDpsNumber(entry.result.hitChance * 100, 2) }}%</td>
            <td>{{ entry.result.maximumHit }}</td>
            <td>{{ formatDpsNumber(entry.result.expectedDamagePerAttack) }}</td>
            <td>{{ entry.scenario.sampleSeed }}</td>
            <td>{{ entry.sampledResult.accuracyPassed ? "Passed" : "Failed" }}</td>
            <td>{{ entry.sampledResult.sampledDamage }}</td>
            <td>{{ formatDpsNumber(entry.result.secondsPerAttack, 1) }}</td>
            <td>{{ formatDpsNumber(entry.result.dps) }}</td>
          </tr>
        </tbody>
      </table>
    </section>

    <section v-else class="loading-panel">
      {{ engineStatusLabel }}
    </section>
  </main>
</template>

<style scoped>
.dps-shell {
    background: #f5f6f4;
    color: #1e252b;
    font-family:
        Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont,
        "Segoe UI", sans-serif;
    min-height: calc(100vh - 56px);
    padding: 28px;
}

.dps-header {
    margin: 0 auto 20px;
    max-width: 1180px;
}

.eyebrow {
    color: #64707a;
    font-size: 0.78rem;
    font-weight: 800;
    letter-spacing: 0;
    margin: 0 0 8px;
    text-transform: uppercase;
}

h1,
p {
    margin: 0;
}

h1 {
    font-size: 2rem;
    line-height: 1.1;
}

.engine-status {
    color: #4d5963;
    font-size: 0.92rem;
    font-weight: 700;
    margin-top: 10px;
}

.engine-status.loaded {
    color: #1d5a43;
}

.engine-status.failed {
    color: #9b2c2c;
}

.dps-table-wrap,
.loading-panel {
    background: #ffffff;
    border: 1px solid #bbc5cf;
    margin: 0 auto;
    max-width: 1180px;
    overflow-x: auto;
}

.dps-table {
    border-collapse: collapse;
    min-width: 1180px;
    width: 100%;
}

th,
td {
    border-bottom: 1px solid #dce2e6;
    padding: 12px 14px;
    text-align: right;
    white-space: nowrap;
}

th:first-child,
td:first-child,
th:nth-child(2),
td:nth-child(2) {
    text-align: left;
}

th {
    background: #eef1ec;
    color: #37424c;
    font-size: 0.78rem;
    letter-spacing: 0;
    text-transform: uppercase;
}

td {
    font-variant-numeric: tabular-nums;
}

.loading-panel {
    color: #4d5963;
    font-weight: 800;
    padding: 24px;
}
</style>
