<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref } from "vue";
import {
    buildDebugTiles,
    defaultFieldOfView,
    getSceneScreenCoordinate,
    readPlayerChaseDebugSnapshot,
    tileSize,
    type CameraMode,
    type DebugTile,
    type PlayerChaseDebugSnapshot,
} from "./playerChaseDebug";
import {
    loadEngineModule,
    type DevelopmentPlayerChaseScenario,
} from "./wasm/EngineModule";

const engineModuleStatus = ref<"loading" | "loaded" | "failed">("loading");
const scenario = ref<DevelopmentPlayerChaseScenario | null>(null);
const snapshot = ref<PlayerChaseDebugSnapshot | null>(null);
const cameraMode = ref<CameraMode>("Follow Player");
const fieldOfView = ref(defaultFieldOfView);
let playbackTimer: number | undefined;

const cameraCenter = computed(() => {
    if (snapshot.value === null) {
        return { x: 15, y: 10, plane: 0 };
    }

    return cameraMode.value === "Follow Player"
        ? snapshot.value.player.coordinate
        : { x: 13, y: 10, plane: 0 };
});
const renderTiles = computed<DebugTile[]>(() =>
    scenario.value === null
        ? []
        : buildDebugTiles(scenario.value, cameraCenter.value, fieldOfView.value),
);
const viewBox = computed(() => {
    const width = getColumnCount(renderTiles.value) * tileSize;
    const height = getRowCount(renderTiles.value) * tileSize;

    return `0 0 ${width} ${height}`;
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
        const module = await loadEngineModule();
        scenario.value = new module.DevelopmentPlayerChaseScenario();
        refreshSnapshot();
        engineModuleStatus.value = "loaded";
    } catch (error) {
        console.error("Failed to load engine wasm module", error);
        engineModuleStatus.value = "failed";
    }
});

onUnmounted(() => {
    stopPlayback();
});

function refreshSnapshot(): void {
    if (scenario.value === null) {
        return;
    }

    snapshot.value = readPlayerChaseDebugSnapshot(
        scenario.value,
        cameraMode.value,
        fieldOfView.value,
    );
}

function toggleRunning(): void {
    if (scenario.value === null) {
        return;
    }

    const running = !scenario.value.IsRunning();
    scenario.value.SetRunning(running);
    refreshSnapshot();

    if (running) {
        playbackTimer = window.setInterval(stepScenario, 700);
    } else {
        stopPlayback();
    }
}

function stopPlayback(): void {
    if (playbackTimer !== undefined) {
        window.clearInterval(playbackTimer);
        playbackTimer = undefined;
    }
}

function stepScenario(): void {
    if (scenario.value === null) {
        return;
    }

    scenario.value.Step();
    refreshSnapshot();
}

function setCameraMode(mode: CameraMode): void {
    cameraMode.value = mode;
    refreshSnapshot();
}

function setFieldOfView(value: number): void {
    fieldOfView.value = value;
    refreshSnapshot();
}

function clickTile(tile: DebugTile): void {
    if (scenario.value === null) {
        return;
    }

    scenario.value.ClickSceneCoordinate(
        tile.coordinate.x,
        tile.coordinate.y,
        tile.coordinate.plane,
    );
    refreshSnapshot();
}

function getTileX(tile: DebugTile): number {
    return getSceneScreenCoordinate(tile.coordinate, renderTiles.value).x;
}

function getTileY(tile: DebugTile): number {
    return getSceneScreenCoordinate(tile.coordinate, renderTiles.value).y;
}

function getColumnCount(tiles: DebugTile[]): number {
    return new Set(tiles.map((tile) => tile.coordinate.x)).size;
}

function getRowCount(tiles: DebugTile[]): number {
    return new Set(tiles.map((tile) => tile.coordinate.y)).size;
}

function formatCoordinate(
    coordinate: { x: number; y: number; plane: number } | null,
): string {
    return coordinate === null
        ? "None"
        : `${coordinate.x}, ${coordinate.y}, P${coordinate.plane}`;
}
</script>

<template>
  <main class="viewer-shell">
    <header class="viewer-header">
      <div>
        <p class="eyebrow">Development viewer</p>
        <h1>Player Chase</h1>
        <p :class="['engine-status', engineModuleStatus]">
          {{ engineStatusLabel }}
        </p>
      </div>

      <div class="controls" v-if="snapshot !== null">
        <button type="button" @click="toggleRunning">
          {{ snapshot.running ? "Pause" : "Run" }}
        </button>
        <button type="button" @click="stepScenario">Step</button>

        <div class="segmented" aria-label="Camera mode">
          <button
            type="button"
            :class="{ selected: cameraMode === 'Follow Player' }"
            @click="setCameraMode('Follow Player')"
          >
            Follow
          </button>
          <button
            type="button"
            :class="{ selected: cameraMode === 'Fixed Scene' }"
            @click="setCameraMode('Fixed Scene')"
          >
            Fixed
          </button>
        </div>

        <label class="fov-control">
          <span>FOV</span>
          <input
            type="range"
            min="8"
            max="18"
            step="2"
            :value="fieldOfView"
            @input="setFieldOfView(Number(($event.target as HTMLInputElement).value))"
          />
          <strong>{{ fieldOfView }}</strong>
        </label>
      </div>
    </header>

    <section v-if="snapshot !== null" class="debug-layout" aria-live="polite">
      <div class="viewport" aria-label="Player Chase scene">
        <svg class="scene-svg" :viewBox="viewBox" role="img">
          <rect
            v-for="tile in renderTiles"
            :key="tile.key"
            :x="getTileX(tile)"
            :y="getTileY(tile)"
            :width="tileSize"
            :height="tileSize"
            :class="['tile', tile.kind]"
            @click="clickTile(tile)"
          />
          <text
            v-for="tile in renderTiles.filter((tile) => tile.kind === 'player' || tile.kind === 'npc')"
            :key="`${tile.key}:label`"
            :x="getTileX(tile) + tileSize / 2"
            :y="getTileY(tile) + tileSize / 2 + 4"
            class="actor-label"
          >
            {{ tile.kind === "player" ? "P" : "N" }}
          </text>
        </svg>
      </div>

      <aside class="details" aria-label="Debug state">
        <dl>
          <div>
            <dt>Tick</dt>
            <dd>{{ snapshot.tick }}</dd>
          </div>
          <div>
            <dt>State</dt>
            <dd>{{ snapshot.running ? "Running" : "Paused" }}</dd>
          </div>
          <div>
            <dt>Camera mode</dt>
            <dd>{{ snapshot.cameraMode }}</dd>
          </div>
          <div>
            <dt>Field of View</dt>
            <dd>{{ snapshot.fieldOfView }}</dd>
          </div>
          <div>
            <dt>Player position</dt>
            <dd>{{ formatCoordinate(snapshot.player.coordinate) }}</dd>
          </div>
          <div>
            <dt>Player Movement Target</dt>
            <dd>{{ formatCoordinate(snapshot.player.movementTarget) }}</dd>
          </div>
          <div>
            <dt>NPC position</dt>
            <dd>{{ formatCoordinate(snapshot.npc.coordinate) }}</dd>
          </div>
          <div>
            <dt>NPC size</dt>
            <dd>{{ snapshot.npc.size }}x{{ snapshot.npc.size }}</dd>
          </div>
          <div>
            <dt>NPC Movement Target</dt>
            <dd>{{ snapshot.npc.movementTarget }}</dd>
          </div>
          <div class="feedback" :class="{ blocked: snapshot.blockedClick }">
            <dt>Blocked click</dt>
            <dd>{{ snapshot.blockedClick ? "Rejected on blocked tile" : "None" }}</dd>
          </div>
          <div>
            <dt>Pathing</dt>
            <dd>{{ snapshot.noPathfindingNote }}</dd>
          </div>
        </dl>
      </aside>
    </section>

    <section v-else class="loading-panel">
      {{ engineStatusLabel }}
    </section>
  </main>
</template>

<style scoped>
.viewer-shell {
    background: #f5f6f4;
    color: #1e252b;
    font-family:
        Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont,
        "Segoe UI", sans-serif;
    min-height: 100vh;
    padding: 28px;
}

.viewer-header {
    align-items: end;
    display: flex;
    gap: 24px;
    justify-content: space-between;
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

.controls {
    align-items: center;
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
    justify-content: flex-end;
}

button {
    background: #ffffff;
    border: 1px solid #bbc5cf;
    border-radius: 6px;
    color: #26313f;
    cursor: pointer;
    font: inherit;
    font-size: 0.92rem;
    font-weight: 800;
    min-height: 38px;
    padding: 0 14px;
}

button.selected,
button:hover {
    background: #1d5a43;
    border-color: #1d5a43;
    color: #ffffff;
}

.segmented {
    display: flex;
    gap: 6px;
}

.fov-control {
    align-items: center;
    background: #ffffff;
    border: 1px solid #bbc5cf;
    border-radius: 6px;
    display: flex;
    gap: 8px;
    min-height: 38px;
    padding: 0 10px;
}

.fov-control span,
.fov-control strong {
    font-size: 0.86rem;
}

.debug-layout {
    display: grid;
    gap: 20px;
    grid-template-columns: minmax(0, 1fr) 360px;
    margin: 0 auto;
    max-width: 1180px;
}

.viewport {
    align-content: center;
    background: #d7ddd6;
    min-height: 620px;
    overflow: auto;
    padding: 18px;
}

.scene-svg {
    background: #eef1ec;
    display: block;
    height: min(74vh, 860px);
    min-height: 560px;
    min-width: 560px;
    width: 100%;
}

.tile {
    cursor: pointer;
    fill: #eef1ec;
    stroke: #b8c3b8;
    stroke-width: 1;
}

.tile:hover {
    fill: #dce6e0;
}

.tile.game-object {
    fill: #6c5c50;
    stroke: #3f332c;
}

.tile.player {
    fill: #1d7d61;
    stroke: #ffffff;
}

.tile.npc {
    fill: #315e9f;
    stroke: #ffffff;
}

.actor-label {
    fill: #ffffff;
    font-size: 12px;
    font-weight: 900;
    pointer-events: none;
    text-anchor: middle;
}

.details,
.loading-panel {
    background: #ffffff;
    border: 1px solid #d1d9df;
    padding: 20px;
}

.details {
    align-self: start;
}

dl {
    display: grid;
    gap: 14px;
    margin: 0;
}

dt {
    color: #61707b;
    font-size: 0.76rem;
    font-weight: 900;
    margin-bottom: 4px;
    text-transform: uppercase;
}

dd {
    color: #1e252b;
    font-size: 0.92rem;
    line-height: 1.42;
    margin: 0;
    overflow-wrap: anywhere;
}

.feedback.blocked dd {
    color: #9b2c2c;
    font-weight: 800;
}

@media (max-width: 900px) {
    .viewer-shell {
        padding: 18px;
    }

    .viewer-header {
        align-items: stretch;
        flex-direction: column;
    }

    .controls {
        justify-content: flex-start;
    }

    .debug-layout {
        grid-template-columns: 1fr;
    }

    .viewport {
        min-height: 440px;
        padding: 12px;
    }

    .scene-svg {
        height: 520px;
    }
}
</style>
