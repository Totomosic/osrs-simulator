<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref, shallowRef } from "vue";
import { loadEngineModule } from "./wasm/EngineModule";
import type { EngineModule, RecordingPlayback } from "./wasm/EngineModule";

const bundledSamples = [
    {
        label: "Minimal Encounter Recording",
        path: "recordings/minimal-encounter-recording.json",
    },
];

const engineModule = shallowRef<EngineModule | null>(null);
const playback = shallowRef<RecordingPlayback | null>(null);
const selectedSamplePath = ref(bundledSamples[0].path);
const loadError = ref("");

const encounterName = computed(
    () => playback.value?.GetEncounterName() ?? "No recording loaded",
);
const secondsPerTick = computed(() => playback.value?.GetSecondsPerTick() ?? 0);
const currentTick = computed(() => playback.value?.GetCurrentTick() ?? 0);
const initialTick = computed(() => playback.value?.GetInitialTick() ?? 0);
const lastTick = computed(() => playback.value?.GetLastTick() ?? 0);
const hasPlayback = computed(() => playback.value !== null);

async function loadSelectedSample(): Promise<void> {
    loadError.value = "";

    try {
        if (engineModule.value === null) {
            engineModule.value = await loadEngineModule();
        }

        const response = await fetch(selectedSamplePath.value);

        if (!response.ok) {
            throw new Error(`Unable to load sample: ${response.status}`);
        }

        const json = await response.text();
        playback.value?.delete?.();
        playback.value = engineModule.value.RecordingPlayback.LoadFromJson(json);
    } catch (error) {
        playback.value = null;
        loadError.value =
            error instanceof Error ? error.message : "Unable to load recording";
    }
}

function previousTick(): void {
    playback.value?.PreviousTick();
}

function nextTick(): void {
    playback.value?.NextTick();
}

onMounted(() => {
    void loadSelectedSample();
});

onUnmounted(() => {
    playback.value?.delete?.();
});
</script>

<template>
  <main class="recording-view">
    <section class="recording-toolbar" aria-label="Recording loader">
      <label>
        Sample
        <select v-model="selectedSamplePath" @change="loadSelectedSample">
          <option
            v-for="sample in bundledSamples"
            :key="sample.path"
            :value="sample.path"
          >
            {{ sample.label }}
          </option>
        </select>
      </label>
      <button type="button" @click="loadSelectedSample">Load</button>
      <p v-if="loadError" class="recording-error" role="alert">
        {{ loadError }}
      </p>
    </section>

    <section class="recording-metadata" aria-label="Recording metadata">
      <h1>{{ encounterName }}</h1>
      <dl>
        <div>
          <dt>Seconds per Tick</dt>
          <dd>{{ secondsPerTick }}</dd>
        </div>
        <div>
          <dt>Timeline</dt>
          <dd>{{ initialTick }}-{{ lastTick }}</dd>
        </div>
      </dl>
    </section>

    <section class="recording-controls" aria-label="Tick navigation">
      <button
        type="button"
        :disabled="!hasPlayback || currentTick <= initialTick"
        @click="previousTick"
      >
        Previous
      </button>
      <output aria-label="Current Tick">Tick {{ currentTick }}</output>
      <button
        type="button"
        :disabled="!hasPlayback || currentTick >= lastTick"
        @click="nextTick"
      >
        Next
      </button>
    </section>
  </main>
</template>

<style scoped>
.recording-view {
    color: #1e252b;
    display: grid;
    gap: 20px;
    margin: 0 auto;
    max-width: 960px;
    padding: 28px;
}

.recording-toolbar,
.recording-controls,
.recording-metadata dl {
    align-items: center;
    display: flex;
    flex-wrap: wrap;
    gap: 12px;
}

.recording-toolbar label {
    align-items: center;
    display: flex;
    font-weight: 800;
    gap: 8px;
}

.recording-toolbar select,
.recording-toolbar button,
.recording-controls button {
    border: 1px solid #8aa0ad;
    border-radius: 6px;
    font: inherit;
    min-height: 36px;
    padding: 6px 10px;
}

.recording-controls output {
    font-weight: 900;
    min-width: 86px;
    text-align: center;
}

.recording-error {
    color: #a31818;
    font-weight: 800;
    margin: 0;
}

.recording-metadata h1 {
    font-size: 1.7rem;
    margin: 0 0 12px;
}

.recording-metadata dl {
    margin: 0;
}

.recording-metadata div {
    border-left: 3px solid #527081;
    padding-left: 10px;
}

.recording-metadata dt {
    color: #52616a;
    font-size: 0.78rem;
    font-weight: 900;
    text-transform: uppercase;
}

.recording-metadata dd {
    font-weight: 800;
    margin: 2px 0 0;
}
</style>
