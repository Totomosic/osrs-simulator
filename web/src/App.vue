<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref, shallowRef } from "vue";
import {
    buildDebugTiles,
    clickDebugTile,
    createDefaultCamera,
    defaultFieldOfView,
    getCameraCenter,
    getSceneScreenCoordinate,
    maxFieldOfView,
    minFieldOfView,
    panCamera,
    readPlayerChaseDebugSnapshot,
    setCameraFieldOfView,
    setCameraMode as setDebugCameraMode,
    tileSize,
    type CameraPanDirection,
    type CameraState,
    type CameraMode,
    type DebugTile,
    type MovementTargetSnapshot,
    type PlayerChaseDebugSnapshot,
} from "./playerChaseDebug";
import {
    loadEngineModule,
    type DevelopmentPlayerChaseScenario,
} from "./wasm/EngineModule";
import { createPlayerChaseScenario } from "./scenarios";
import {
    createPlayerChasePlayback,
    type PlayerChasePlaybackControls,
} from "./playerChasePlayback";

const engineModuleStatus = ref<"loading" | "loaded" | "failed">("loading");
const scenario = shallowRef<DevelopmentPlayerChaseScenario | null>(null);
const snapshot = ref<PlayerChaseDebugSnapshot | null>(null);
const camera = ref<CameraState | null>(null);
let playback: PlayerChasePlaybackControls | null = null;

const cameraCenter = computed(() => {
    if (snapshot.value === null || camera.value === null) {
        return { x: 15, y: 10, plane: 0 };
    }

    return getCameraCenter(camera.value, snapshot.value);
});
const renderTiles = computed<DebugTile[]>(() =>
    snapshot.value === null || camera.value === null
        ? []
        : buildDebugTiles(
              snapshot.value,
              cameraCenter.value,
              camera.value.fieldOfView,
          ),
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
    window.addEventListener("keydown", handleCameraKeyDown);

    try {
        const module = await loadEngineModule();
        scenario.value = createPlayerChaseScenario(module);
        playback = createPlayerChasePlayback(scenario.value, refreshSnapshot);
        refreshSnapshot();
        engineModuleStatus.value = "loaded";
    } catch (error) {
        console.error("Failed to load engine wasm module", error);
        engineModuleStatus.value = "failed";
    }
});

onUnmounted(() => {
    window.removeEventListener("keydown", handleCameraKeyDown);
    playback?.stop();
});

function refreshSnapshot(): void {
    if (scenario.value === null) {
        return;
    }

    const nextSnapshot = readPlayerChaseDebugSnapshot(
        scenario.value,
        camera.value?.mode ?? "Follow Player",
        camera.value?.fieldOfView ?? defaultFieldOfView,
    );

    if (camera.value === null) {
        camera.value = createDefaultCamera(nextSnapshot);
        snapshot.value = readPlayerChaseDebugSnapshot(
            scenario.value,
            camera.value.mode,
            camera.value.fieldOfView,
        );
        return;
    }

    snapshot.value = nextSnapshot;
}

function toggleRunning(): void {
    playback?.toggle();
}

function stepWhilePaused(): void {
    playback?.stepWhilePaused();
}

function setCameraMode(mode: CameraMode): void {
    if (scenario.value === null || snapshot.value === null || camera.value === null) {
        return;
    }

    camera.value = setDebugCameraMode(
        camera.value,
        mode,
        snapshot.value,
    );
    refreshSnapshot();
}

function setFieldOfView(value: number): void {
    if (scenario.value === null || snapshot.value === null || camera.value === null) {
        return;
    }

    camera.value = setCameraFieldOfView(
        camera.value,
        value,
        snapshot.value,
    );
    refreshSnapshot();
}

function pan(direction: CameraPanDirection): void {
    if (scenario.value === null || snapshot.value === null || camera.value === null) {
        return;
    }

    camera.value = panCamera(
        camera.value,
        direction,
        snapshot.value,
    );
    refreshSnapshot();
}

function handleCameraKeyDown(event: KeyboardEvent): void {
    if (event.target instanceof HTMLInputElement) {
        return;
    }

    const direction = getKeyboardPanDirection(event.key);

    if (direction === null) {
        return;
    }

    event.preventDefault();
    pan(direction);
}

function getKeyboardPanDirection(key: string): CameraPanDirection | null {
    switch (key) {
        case "ArrowUp":
        case "w":
        case "W":
            return "north";
        case "ArrowRight":
        case "d":
        case "D":
            return "east";
        case "ArrowDown":
        case "s":
        case "S":
            return "south";
        case "ArrowLeft":
        case "a":
        case "A":
            return "west";
        default:
            return null;
    }
}

function wheelFieldOfView(event: WheelEvent): void {
    if (camera.value === null) {
        return;
    }

    const delta = event.deltaY > 0 ? 2 : -2;
    setFieldOfView(camera.value.fieldOfView + delta);
}

function clickTile(tile: DebugTile): void {
    if (scenario.value === null) {
        return;
    }

    clickDebugTile(scenario.value, tile);
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

function formatMovementTarget(target: MovementTargetSnapshot | null): string {
    if (target === null) {
        return "None";
    }

    if (target.kind === "SceneCoordinate" && target.coordinate !== undefined) {
        return formatCoordinate(target.coordinate);
    }

    return target.label ?? `Actor #${target.actorId ?? 0}`;
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
        <button type="button" :disabled="snapshot.running" @click="stepWhilePaused">
          Step
        </button>

        <div class="segmented" aria-label="Camera mode">
          <button
            type="button"
            :class="{ selected: camera?.mode === 'Follow Player' }"
            @click="setCameraMode('Follow Player')"
          >
            Player
          </button>
          <button
            type="button"
            :class="{ selected: camera?.mode === 'Follow NPC' }"
            @click="setCameraMode('Follow NPC')"
          >
            NPC
          </button>
          <button
            type="button"
            :class="{ selected: camera?.mode === 'Free Camera' }"
            @click="setCameraMode('Free Camera')"
          >
            Free
          </button>
        </div>

        <div class="pan-controls" aria-label="Pan Camera">
          <button type="button" @click="pan('north')">N</button>
          <button type="button" @click="pan('west')">W</button>
          <button type="button" @click="pan('east')">E</button>
          <button type="button" @click="pan('south')">S</button>
        </div>

        <label class="fov-control">
          <span>FOV</span>
          <input
            type="range"
            :min="minFieldOfView"
            :max="maxFieldOfView"
            step="2"
            :value="camera?.fieldOfView ?? defaultFieldOfView"
            @input="setFieldOfView(Number(($event.target as HTMLInputElement).value))"
          />
          <strong>{{ camera?.fieldOfView ?? defaultFieldOfView }}</strong>
        </label>
      </div>
    </header>

    <section v-if="snapshot !== null" class="debug-layout" aria-live="polite">
      <div
        class="viewport"
        aria-label="Player Chase scene"
        @wheel.prevent="wheelFieldOfView"
      >
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
          <text
            v-for="tile in renderTiles.filter((tile) => tile.flags.includes('BlockMovementObject'))"
            :key="`${tile.key}:movement-flag`"
            :x="getTileX(tile) + tileSize - 6"
            :y="getTileY(tile) + 10"
            class="flag-label"
          >
            M
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
            <dd>{{ formatMovementTarget(snapshot.player.movementTarget) }}</dd>
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
            <dd>{{ formatMovementTarget(snapshot.npc.movementTarget) }}</dd>
          </div>
          <div>
            <dt>Tile Flags</dt>
            <dd>
              {{
                snapshot.tiles
                  .filter((tile) => tile.flags.includes("BlockMovementObject"))
                  .length
              }}
              movement-blocking object tiles
            </dd>
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

.pan-controls {
    display: grid;
    gap: 4px;
    grid-template-columns: repeat(4, 38px);
}

.pan-controls button {
    padding: 0;
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

.flag-label {
    fill: #ffffff;
    font-size: 9px;
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
