<script setup lang="ts">
import { computed, onMounted, reactive, ref } from "vue";
import {
    addPlayerAttackSetup,
    buildSetupResultRows,
    createDefaultCalculatorState,
    deletePlayerAttackSetup,
    getActivePlayerAttackSetup,
    isMeleeAttackType,
    isRangedAttackType,
    maxPlayerAttackSetups,
    setPlayerAttackType,
    type CalculatorAttackType,
    type CombatStylePreset,
    type DpsResultRow,
} from "./dpsCalculator";
import {
    loadEngineModule,
    type EngineModule,
} from "./wasm/EngineModule";

const engineModuleStatus = ref<"loading" | "loaded" | "failed">("loading");
const engineModule = ref<EngineModule | null>(null);
const calculatorState = reactive(createDefaultCalculatorState());
const activePlayerAttackSetup = computed(() =>
    getActivePlayerAttackSetup(calculatorState),
);

const attackTypeOptions: Array<{ value: CalculatorAttackType; label: string }> = [
    { value: "stab", label: "Stab" },
    { value: "slash", label: "Slash" },
    { value: "crush", label: "Crush" },
    { value: "magic", label: "Magic" },
    { value: "ranged-light", label: "Ranged light" },
    { value: "ranged-standard", label: "Ranged standard" },
    { value: "ranged-heavy", label: "Ranged heavy" },
];

const meleeCombatStyleOptions: Array<{
    value: CombatStylePreset;
    label: string;
}> = [
    { value: "melee-accurate", label: "Accurate" },
    { value: "melee-aggressive", label: "Aggressive" },
    { value: "melee-controlled", label: "Controlled" },
    { value: "melee-defensive", label: "Defensive" },
];

const rangedCombatStyleOptions: Array<{
    value: CombatStylePreset;
    label: string;
}> = [
    { value: "ranged-accurate", label: "Accurate" },
    { value: "ranged-rapid", label: "Rapid" },
    { value: "ranged-longrange", label: "Longrange" },
];

const magicCombatStyleOptions: Array<{
    value: CombatStylePreset;
    label: string;
}> = [
    { value: "magic-standard", label: "Standard" },
    { value: "magic-defensive", label: "Defensive casting" },
];

const isMeleeSetup = computed(() =>
    isMeleeAttackType(activePlayerAttackSetup.value.attackType),
);
const isRangedSetup = computed(() =>
    isRangedAttackType(activePlayerAttackSetup.value.attackType),
);
const isMagicSetup = computed(
    () => activePlayerAttackSetup.value.attackType === "magic",
);
const selectedMeleeAttackBonusLabel = computed(() => {
    switch (activePlayerAttackSetup.value.attackType) {
        case "stab":
            return "Stab attack bonus";
        case "slash":
            return "Slash attack bonus";
        case "crush":
            return "Crush attack bonus";
        default:
            return "Attack bonus";
    }
});
const combatStyleOptions = computed(() => {
    if (isRangedSetup.value) {
        return rangedCombatStyleOptions;
    }

    if (isMagicSetup.value) {
        return magicCombatStyleOptions;
    }

    return meleeCombatStyleOptions;
});

const resultRows = computed<DpsResultRow[]>(() => {
    if (engineModule.value === null) {
        return [];
    }

    return buildSetupResultRows(engineModule.value, calculatorState);
});

const canAddPlayerAttackSetup = computed(
    () => calculatorState.playerAttackSetups.length < maxPlayerAttackSetups,
);

const canDeletePlayerAttackSetup = computed(
    () => calculatorState.playerAttackSetups.length > 1,
);

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

function onAttackTypeChange(event: Event): void {
    const select = event.target as HTMLSelectElement;
    setPlayerAttackType(
        activePlayerAttackSetup.value,
        select.value as CalculatorAttackType,
    );
}

function onAddPlayerAttackSetup(): void {
    addPlayerAttackSetup(calculatorState);
}

function onDeleteActivePlayerAttackSetup(): void {
    deletePlayerAttackSetup(
        calculatorState,
        calculatorState.activePlayerAttackSetupIndex,
    );
}

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
      <form class="setup-panel player-setup-panel" aria-label="Player attack setup">
        <div class="panel-heading">
          <p class="eyebrow">Player attack setup</p>
          <div class="setup-tabs" role="tablist" aria-label="Player attack setups">
            <button
              v-for="(setup, setupIndex) in calculatorState.playerAttackSetups"
              :key="setupIndex"
              :class="[
                'setup-tab',
                setupIndex === calculatorState.activePlayerAttackSetupIndex
                  ? 'active'
                  : '',
              ]"
              type="button"
              role="tab"
              :aria-selected="setupIndex === calculatorState.activePlayerAttackSetupIndex"
              @click="calculatorState.activePlayerAttackSetupIndex = setupIndex"
            >
              {{ setup.name }}
            </button>
            <button
              class="setup-tab action"
              type="button"
              :disabled="!canAddPlayerAttackSetup"
              @click="onAddPlayerAttackSetup"
            >
              Add
            </button>
            <button
              class="setup-tab action"
              type="button"
              :disabled="!canDeletePlayerAttackSetup"
              @click="onDeleteActivePlayerAttackSetup"
            >
              Delete
            </button>
          </div>
          <input
            v-model="activePlayerAttackSetup.name"
            class="setup-name"
            aria-label="Setup name"
          >
        </div>

        <label class="wide-field">
          <span>Attack type</span>
          <select
            :value="activePlayerAttackSetup.attackType"
            @change="onAttackTypeChange"
          >
            <option
              v-for="option in attackTypeOptions"
              :key="option.value"
              :value="option.value"
            >
              {{ option.label }}
            </option>
          </select>
        </label>

        <label v-if="isMeleeSetup">
          <span>Attack</span>
          <input
            v-model.number="activePlayerAttackSetup.attack"
            min="1"
            step="1"
            type="number"
          >
        </label>

        <label v-if="isMeleeSetup">
          <span>Strength</span>
          <input
            v-model.number="activePlayerAttackSetup.strength"
            min="1"
            step="1"
            type="number"
          >
        </label>

        <label v-if="isRangedSetup">
          <span>Ranged</span>
          <input
            v-model.number="activePlayerAttackSetup.ranged"
            min="1"
            step="1"
            type="number"
          >
        </label>

        <label v-if="isMagicSetup">
          <span>Magic</span>
          <input
            v-model.number="activePlayerAttackSetup.magic"
            min="1"
            step="1"
            type="number"
          >
        </label>

        <label v-if="activePlayerAttackSetup.attackType === 'stab'">
          <span>{{ selectedMeleeAttackBonusLabel }}</span>
          <input
            v-model.number="activePlayerAttackSetup.stabAttack"
            step="1"
            type="number"
          >
        </label>

        <label v-if="activePlayerAttackSetup.attackType === 'slash'">
          <span>{{ selectedMeleeAttackBonusLabel }}</span>
          <input
            v-model.number="activePlayerAttackSetup.slashAttack"
            step="1"
            type="number"
          >
        </label>

        <label v-if="activePlayerAttackSetup.attackType === 'crush'">
          <span>{{ selectedMeleeAttackBonusLabel }}</span>
          <input
            v-model.number="activePlayerAttackSetup.crushAttack"
            step="1"
            type="number"
          >
        </label>

        <label v-if="isRangedSetup">
          <span>Ranged attack bonus</span>
          <input
            v-model.number="activePlayerAttackSetup.rangedAttack"
            step="1"
            type="number"
          >
        </label>

        <label v-if="isMagicSetup">
          <span>Magic attack bonus</span>
          <input
            v-model.number="activePlayerAttackSetup.magicAttack"
            step="1"
            type="number"
          >
        </label>

        <label v-if="isMeleeSetup">
          <span>Melee strength</span>
          <input
            v-model.number="activePlayerAttackSetup.meleeStrength"
            step="1"
            type="number"
          >
        </label>

        <label v-if="isRangedSetup">
          <span>Ranged strength</span>
          <input
            v-model.number="activePlayerAttackSetup.rangedStrength"
            step="1"
            type="number"
          >
        </label>

        <label v-if="isMagicSetup">
          <span>Magic damage percent</span>
          <input
            v-model.number="activePlayerAttackSetup.magicDamagePercent"
            min="0"
            step="0.1"
            type="number"
          >
        </label>

        <label v-if="isMagicSetup">
          <span>Magic base maximum hit</span>
          <input
            v-model.number="activePlayerAttackSetup.magicBaseMaximumHit"
            min="0"
            step="1"
            type="number"
          >
        </label>

        <label class="wide-field">
          <span>Combat style</span>
          <select v-model="activePlayerAttackSetup.combatStylePreset">
            <option
              v-for="option in combatStyleOptions"
              :key="option.value"
              :value="option.value"
            >
              {{ option.label }}
            </option>
          </select>
        </label>

        <label>
          <span>Weapon speed (ticks)</span>
          <input
            v-model.number="activePlayerAttackSetup.weaponSpeedTicks"
            min="1"
            step="1"
            type="number"
          >
        </label>
      </form>

      <form class="setup-panel npc-setup-panel" aria-label="NPC defence setup">
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
          <span>Magic</span>
          <input
            v-model.number="calculatorState.npcDefenceSetup.magic"
            min="1"
            step="1"
            type="number"
          >
        </label>

        <label>
          <span>Stab defence bonus</span>
          <input
            v-model.number="calculatorState.npcDefenceSetup.stabDefence"
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

        <label>
          <span>Crush defence bonus</span>
          <input
            v-model.number="calculatorState.npcDefenceSetup.crushDefence"
            step="1"
            type="number"
          >
        </label>

        <label>
          <span>Magic defence bonus</span>
          <input
            v-model.number="calculatorState.npcDefenceSetup.magicDefence"
            step="1"
            type="number"
          >
        </label>

        <label>
          <span>Ranged light defence bonus</span>
          <input
            v-model.number="calculatorState.npcDefenceSetup.rangedDefenceLight"
            step="1"
            type="number"
          >
        </label>

        <label>
          <span>Ranged standard defence bonus</span>
          <input
            v-model.number="calculatorState.npcDefenceSetup.rangedDefenceStandard"
            step="1"
            type="number"
          >
        </label>

        <label>
          <span>Ranged heavy defence bonus</span>
          <input
            v-model.number="calculatorState.npcDefenceSetup.rangedDefenceHeavy"
            step="1"
            type="number"
          >
        </label>
      </form>

      <section v-if="resultRows.length > 0" class="dps-table-wrap dps-results-panel">
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
              <th>DPS Diff</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="(row, rowIndex) in resultRows" :key="rowIndex">
              <td>{{ row.name }}</td>
              <td>{{ row.attackRoll }}</td>
              <td>{{ row.defenceRoll }}</td>
              <td>{{ row.hitChance }}</td>
              <td>{{ row.maximumHit }}</td>
              <td>{{ row.expectedDamagePerAttack }}</td>
              <td>{{ row.secondsPerAttack }}</td>
              <td>{{ row.dps }}</td>
              <td>{{ row.dpsDifference }}</td>
            </tr>
          </tbody>
        </table>
      </section>

      <section v-else class="loading-panel dps-results-panel">
        {{ engineStatusLabel }}
      </section>
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

*,
*::before,
*::after {
    box-sizing: border-box;
}

.dps-header,
.calculator-grid {
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
    grid-template-areas:
        "player npc"
        "results results";
    grid-template-columns: minmax(0, 1.45fr) minmax(320px, 0.95fr);
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
    grid-template-columns: repeat(2, minmax(0, 1fr));
    padding: 16px;
}

.player-setup-panel {
    grid-area: player;
}

.npc-setup-panel {
    grid-area: npc;
    grid-template-columns: repeat(2, minmax(0, 1fr));
}

.dps-results-panel {
    grid-area: results;
    margin: 0;
    max-width: none;
    min-width: 0;
    width: 100%;
}

.panel-heading {
    grid-column: 1 / -1;
}

.setup-tabs {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
    margin-bottom: 12px;
}

.setup-tab {
    background: #eef1ec;
    border: 1px solid #aeb9c2;
    color: #26313a;
    cursor: pointer;
    font: inherit;
    font-size: 0.82rem;
    font-weight: 800;
    max-width: 100%;
    min-height: 34px;
    overflow-wrap: anywhere;
    padding: 6px 10px;
}

.setup-tab.active {
    background: #1f4d3d;
    border-color: #1f4d3d;
    color: #ffffff;
}

.setup-tab.action {
    background: #fbfcfa;
}

.setup-tab:disabled {
    color: #8b969f;
    cursor: not-allowed;
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
    min-width: 0;
}

input,
select {
    background: #fbfcfa;
    border: 1px solid #aeb9c2;
    color: #1e252b;
    font: inherit;
    min-width: 0;
    padding: 8px 10px;
    width: 100%;
}

.wide-field {
    grid-column: 1 / -1;
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
    padding: 10px 12px;
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

    .calculator-grid {
        grid-template-areas:
            "player"
            "npc"
            "results";
        grid-template-columns: minmax(0, 1fr);
    }

    .setup-panel {
        grid-template-columns: 1fr;
    }
}
</style>
