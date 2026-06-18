<script setup lang="ts">
import { computed, onMounted, reactive, ref } from "vue";
import {
    buildSingleSetupResultRow,
    createDefaultCalculatorState,
    type DpsResultRow,
} from "./dpsCalculator";
import {
    loadEngineModule,
    type EngineModule,
} from "./wasm/EngineModule";

const engineModuleStatus = ref<"loading" | "loaded" | "failed">("loading");
const engineModule = ref<EngineModule | null>(null);
const calculatorState = reactive(createDefaultCalculatorState());

const resultRow = computed<DpsResultRow | null>(() => {
    if (engineModule.value === null) {
        return null;
    }

    return buildSingleSetupResultRow(engineModule.value, calculatorState);
});

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
        engineModule.value = await loadEngineModule();
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

    <section class="calculator-grid" aria-label="NPC DPS calculator">
      <form class="setup-panel" aria-label="Player attack setup">
        <div class="panel-heading">
          <p class="eyebrow">Player attack setup</p>
          <input
            v-model="calculatorState.playerAttackSetup.name"
            class="setup-name"
            aria-label="Setup name"
          >
        </div>

        <label>
          <span>Attack</span>
          <input
            v-model.number="calculatorState.playerAttackSetup.attack"
            min="1"
            step="1"
            type="number"
          >
        </label>

        <label>
          <span>Strength</span>
          <input
            v-model.number="calculatorState.playerAttackSetup.strength"
            min="1"
            step="1"
            type="number"
          >
        </label>

        <label>
          <span>Slash attack bonus</span>
          <input
            v-model.number="calculatorState.playerAttackSetup.slashAttack"
            step="1"
            type="number"
          >
        </label>

        <label>
          <span>Melee strength</span>
          <input
            v-model.number="calculatorState.playerAttackSetup.meleeStrength"
            step="1"
            type="number"
          >
        </label>

        <label>
          <span>Attack style bonus</span>
          <input
            v-model.number="calculatorState.playerAttackSetup.attackStyleBonus"
            step="1"
            type="number"
          >
        </label>

        <label>
          <span>Strength style bonus</span>
          <input
            v-model.number="calculatorState.playerAttackSetup.strengthStyleBonus"
            step="1"
            type="number"
          >
        </label>

        <label>
          <span>Weapon speed (ticks)</span>
          <input
            v-model.number="calculatorState.playerAttackSetup.weaponSpeedTicks"
            min="1"
            step="1"
            type="number"
          >
        </label>
      </form>

      <form class="setup-panel" aria-label="NPC defence setup">
        <div class="panel-heading">
          <p class="eyebrow">NPC defence setup</p>
          <h2>NPC</h2>
        </div>

        <label>
          <span>Defence</span>
          <input
            v-model.number="calculatorState.npcDefenceSetup.defence"
            min="1"
            step="1"
            type="number"
          >
        </label>

        <label>
          <span>Slash defence bonus</span>
          <input
            v-model.number="calculatorState.npcDefenceSetup.slashDefence"
            step="1"
            type="number"
          >
        </label>
      </form>
    </section>

    <section v-if="resultRow" class="dps-table-wrap">
      <table class="dps-table">
        <thead>
          <tr>
            <th>Setup</th>
            <th>Attack Roll</th>
            <th>Defence Roll</th>
            <th>Hit Chance</th>
            <th>Maximum Hit</th>
            <th>Expected Damage</th>
            <th>Seconds</th>
            <th>DPS</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td>{{ resultRow.name }}</td>
            <td>{{ resultRow.attackRoll }}</td>
            <td>{{ resultRow.defenceRoll }}</td>
            <td>{{ resultRow.hitChance }}</td>
            <td>{{ resultRow.maximumHit }}</td>
            <td>{{ resultRow.expectedDamagePerAttack }}</td>
            <td>{{ resultRow.secondsPerAttack }}</td>
            <td>{{ resultRow.dps }}</td>
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

.dps-header,
.calculator-grid,
.dps-table-wrap,
.loading-panel {
    margin: 0 auto;
    max-width: 1180px;
}

.dps-header {
    margin-bottom: 18px;
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
h2,
p {
    margin: 0;
}

h1 {
    font-size: 2rem;
    line-height: 1.1;
}

h2 {
    font-size: 1.1rem;
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

.calculator-grid {
    display: grid;
    gap: 16px;
    grid-template-columns: minmax(0, 2fr) minmax(280px, 1fr);
    margin-bottom: 16px;
}

.setup-panel,
.dps-table-wrap,
.loading-panel {
    background: #ffffff;
    border: 1px solid #bbc5cf;
}

.setup-panel {
    display: grid;
    gap: 12px;
    grid-template-columns: repeat(2, minmax(160px, 1fr));
    padding: 16px;
}

.panel-heading {
    grid-column: 1 / -1;
}

.setup-name {
    font-size: 1.1rem;
    font-weight: 800;
}

label {
    color: #37424c;
    display: grid;
    font-size: 0.82rem;
    font-weight: 800;
    gap: 6px;
}

input {
    background: #fbfcfa;
    border: 1px solid #aeb9c2;
    color: #1e252b;
    font: inherit;
    min-width: 0;
    padding: 8px 10px;
    width: 100%;
}

.dps-table-wrap {
    overflow-x: auto;
}

.dps-table {
    border-collapse: collapse;
    min-width: 860px;
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
td:first-child {
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

@media (max-width: 760px) {
    .dps-shell {
        padding: 18px;
    }

    .calculator-grid,
    .setup-panel {
        grid-template-columns: 1fr;
    }
}
</style>
