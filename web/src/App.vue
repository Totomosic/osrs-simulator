<script setup lang="ts">
import { computed, onMounted, ref } from "vue";
import {
    buildRenderTiles,
    getSceneScreenCoordinate,
    runScenario,
    scenarioOptions,
    type CardinalDirection,
    type RenderTile,
    type ScenarioId,
    type ScenarioResult,
} from "./scenarios";
import { loadEngineModule } from "./wasm/EngineModule";

interface SvgTile extends RenderTile {
    screenX: number;
    screenY: number;
    hasObjectBlocker: boolean;
    hasFullBlocker: boolean;
}

interface SvgEdge {
    key: string;
    x1: number;
    y1: number;
    x2: number;
    y2: number;
    wall: boolean;
}

const tileSize = 10;
const engineModuleStatus = ref<"loading" | "loaded" | "failed">("loading");
const engineLoadedTick = ref<number | null>(null);
const defaultSceneId = ref<number | null>(null);
const selectedScenarioId = ref<ScenarioId>("empty-world");
const selectedPlane = ref(0);
const scenario = computed<ScenarioResult>(() => runScenario(selectedScenarioId.value));
const viewBox = computed(
    () =>
        `0 0 ${scenario.value.scene.width * tileSize} ${
            scenario.value.scene.height * tileSize
        }`,
);
const renderTiles = computed<SvgTile[]>(() =>
    buildRenderTiles(scenario.value, selectedPlane.value).map((tile) => {
        const screenCoordinate = getSceneScreenCoordinate(
            tile.coordinate,
            scenario.value.scene,
            tileSize,
        );

        return {
            ...tile,
            screenX: screenCoordinate.x,
            screenY: screenCoordinate.y,
            hasObjectBlocker: tile.flags.includes("BlockMovementObject"),
            hasFullBlocker:
                tile.flags.includes("BlockMovementFull") ||
                tile.flags.includes("BlockLineOfSightFull"),
        };
    }),
);
const objectTiles = computed(() =>
    renderTiles.value.filter((tile) => tile.hasObjectBlocker || tile.hasFullBlocker),
);
const actorTiles = computed(() =>
    renderTiles.value.filter((tile) => tile.actorLabel !== undefined),
);
const movementEdges = computed(() => buildEdges(renderTiles.value, false));
const wallEdges = computed(() => buildEdges(renderTiles.value, true));
const sceneContentSummary = computed(() => {
    const wallObjects = scenario.value.wallObjects.map(
        (wallObject) =>
            `Wall Object #${wallObject.id} at ${wallObject.coordinate.x},${wallObject.coordinate.y} facing ${wallObject.directions.join("/")}`,
    );
    const gameObjects = scenario.value.gameObjects.map(
        (gameObject) =>
            `Game Object #${gameObject.id} at ${gameObject.origin.x},${gameObject.origin.y}, size ${gameObject.sizeX}x${gameObject.sizeY}`,
    );

    return [...wallObjects, ...gameObjects].join(" | ") || "None";
});
const engineModuleStatusLabel = computed(() => {
    switch (engineModuleStatus.value) {
        case "loaded":
            return `Engine wasm loaded | Tick ${engineLoadedTick.value} | Scene #${defaultSceneId.value}`;
        case "failed":
            return "Engine wasm unavailable";
        case "loading":
            return "Loading engine wasm";
    }
});

onMounted(async () => {
    try {
        const module = await loadEngineModule();
        const engine = new module.Engine();
        const world = new module.World();

        engine.Step();
        engineLoadedTick.value = engine.GetCurrentTick();
        defaultSceneId.value = world.GetDefaultSceneId();
        engineModuleStatus.value = "loaded";
    } catch (error) {
        console.error("Failed to load engine wasm module", error);
        engineModuleStatus.value = "failed";
    }
});

function selectScenario(id: ScenarioId): void {
    selectedScenarioId.value = id;
}

function selectPlane(plane: number): void {
    selectedPlane.value = plane;
}

function buildEdges(tiles: SvgTile[], wallOnly: boolean): SvgEdge[] {
    return tiles.flatMap((tile) =>
        (wallOnly ? tile.wallEdges : tile.movementEdges).map((direction) =>
            buildEdge(tile, direction, wallOnly),
        ),
    );
}

function buildEdge(
    tile: SvgTile,
    direction: CardinalDirection,
    wall: boolean,
): SvgEdge {
    const x = tile.screenX;
    const y = tile.screenY;
    const maxX = x + tileSize;
    const maxY = y + tileSize;
    const key = `${tile.key}:${wall ? "wall" : "movement"}:${direction}`;

    switch (direction) {
        case "North":
            return { key, x1: x, y1: y, x2: maxX, y2: y, wall };
        case "East":
            return { key, x1: maxX, y1: y, x2: maxX, y2: maxY, wall };
        case "South":
            return { key, x1: x, y1: maxY, x2: maxX, y2: maxY, wall };
        case "West":
            return { key, x1: x, y1: y, x2: x, y2: maxY, wall };
    }
}
</script>

<template>
  <main class="viewer-shell">
    <header class="viewer-header">
      <div>
        <p class="eyebrow">Development viewer</p>
        <h1>OSRS Simulator</h1>
        <p :class="['engine-status', engineModuleStatus]">
          {{ engineModuleStatusLabel }}
        </p>
      </div>

      <div class="control-stack">
        <div class="scenario-tabs" aria-label="Movement verification scenarios">
          <button
            v-for="option in scenarioOptions"
            :key="option.id"
            type="button"
            :class="{ selected: option.id === selectedScenarioId }"
            @click="selectScenario(option.id)"
          >
            {{ option.title }}
          </button>
        </div>

        <div class="plane-tabs" aria-label="Scene plane">
          <button
            v-for="plane in scenario.scene.planeCount"
            :key="plane - 1"
            type="button"
            :class="{ selected: plane - 1 === selectedPlane }"
            @click="selectPlane(plane - 1)"
          >
            P{{ plane - 1 }}
          </button>
        </div>
      </div>
    </header>

    <section class="scenario-layout" aria-live="polite">
      <div class="viewport" aria-label="Scene tiles">
        <svg class="scene-svg" :viewBox="viewBox" role="img">
          <defs>
            <pattern
              id="scene-grid"
              :width="tileSize"
              :height="tileSize"
              patternUnits="userSpaceOnUse"
            >
              <path
                :d="`M ${tileSize} 0 L 0 0 0 ${tileSize}`"
                class="grid-line"
              />
            </pattern>
          </defs>

          <rect
            class="scene-background"
            x="0"
            y="0"
            :width="scenario.scene.width * tileSize"
            :height="scenario.scene.height * tileSize"
          />
          <rect
            class="scene-grid-fill"
            x="0"
            y="0"
            :width="scenario.scene.width * tileSize"
            :height="scenario.scene.height * tileSize"
          />

          <rect
            v-for="tile in objectTiles"
            :key="`${tile.key}:object`"
            :x="tile.screenX"
            :y="tile.screenY"
            :width="tileSize"
            :height="tileSize"
            :class="[
              'object-tile',
              { 'full-blocker': tile.hasFullBlocker, 'game-object': tile.gameObject !== undefined },
            ]"
          />

          <rect
            v-for="tile in actorTiles"
            :key="`${tile.key}:actor`"
            :x="tile.screenX + 1"
            :y="tile.screenY + 1"
            :width="tileSize - 2"
            :height="tileSize - 2"
            :class="['actor-tile', tile.actorKind === 'Player' ? 'player' : 'npc']"
          />

          <line
            v-for="edge in movementEdges"
            :key="edge.key"
            :x1="edge.x1"
            :y1="edge.y1"
            :x2="edge.x2"
            :y2="edge.y2"
            class="movement-edge"
          />

          <line
            v-for="edge in wallEdges"
            :key="edge.key"
            :x1="edge.x1"
            :y1="edge.y1"
            :x2="edge.x2"
            :y2="edge.y2"
            class="wall-edge"
          />
        </svg>
      </div>

      <aside class="details" aria-labelledby="scenario-title">
        <p class="eyebrow">Scenario</p>
        <h2 id="scenario-title">{{ scenario.title }}</h2>
        <p>{{ scenario.description }}</p>

        <dl>
          <div>
            <dt>Movement outcome</dt>
            <dd>{{ scenario.movementOutcome }}</dd>
          </div>
          <div>
            <dt>Actor Footprint</dt>
            <dd>
              <span v-for="actor in scenario.actors" :key="actor.id">
                {{ actor.kind }} #{{ actor.id }} on Scene #{{ actor.sceneId }}:
                {{ actor.footprint.map((tile) => `${tile.x},${tile.y}`).join(" | ") }}
              </span>
            </dd>
          </div>
          <div>
            <dt>Tile Flags</dt>
            <dd>
              {{
                renderTiles
                  .filter((tile) => tile.flags.length > 0)
                  .map((tile) => `${tile.coordinate.x},${tile.coordinate.y}: ${tile.flags.join("/")}`)
                  .join(" | ") || "None"
              }}
            </dd>
          </div>
          <div>
            <dt>Scene content</dt>
            <dd>{{ sceneContentSummary }}</dd>
          </div>
        </dl>
      </aside>
    </section>
  </main>
</template>

<style scoped>
.viewer-shell {
    background: #f4f6f1;
    color: #1f2933;
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
    margin: 0 auto 24px;
    max-width: 1240px;
}

.eyebrow {
    color: #58606c;
    font-size: 0.78rem;
    font-weight: 700;
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

.engine-status {
    color: #4d5763;
    font-size: 0.92rem;
    font-weight: 700;
    margin-top: 10px;
}

.engine-status.loaded {
    color: #1d4f45;
}

.engine-status.failed {
    color: #9b2c2c;
}

h2 {
    font-size: 1.35rem;
    line-height: 1.2;
    margin-bottom: 10px;
}

.control-stack {
    display: grid;
    gap: 10px;
    justify-items: end;
}

.scenario-tabs,
.plane-tabs {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
    justify-content: flex-end;
}

button {
    background: #ffffff;
    border: 1px solid #c8d0d9;
    border-radius: 6px;
    color: #26313f;
    cursor: pointer;
    font: inherit;
    font-size: 0.92rem;
    font-weight: 700;
    min-height: 40px;
    padding: 0 14px;
}

.plane-tabs button {
    min-width: 44px;
    padding: 0 10px;
}

button.selected {
    background: #1d4f45;
    border-color: #1d4f45;
    color: #ffffff;
}

.scenario-layout {
    display: grid;
    gap: 24px;
    grid-template-columns: minmax(0, 1fr) 360px;
    margin: 0 auto;
    max-width: 1240px;
}

.viewport {
    align-content: center;
    background: #d6ddd0;
    min-height: 620px;
    overflow: auto;
    padding: 20px;
}

.scene-svg {
    background: #eef1ea;
    display: block;
    height: min(74vh, 920px);
    min-height: 560px;
    min-width: 820px;
    width: 100%;
}

.scene-background {
    fill: #eef1ea;
}

.scene-grid-fill {
    fill: url("#scene-grid");
}

.grid-line {
    fill: none;
    stroke: #b9c4b4;
    stroke-width: 0.7;
}

.object-tile {
    fill: #7e6959;
    opacity: 0.82;
    stroke: #40352f;
    stroke-width: 0.8;
}

.object-tile.full-blocker {
    fill: #6a584c;
}

.object-tile.game-object {
    stroke: #c07a2a;
    stroke-width: 1.2;
}

.actor-tile {
    opacity: 0.88;
    stroke: #ffffff;
    stroke-width: 0.6;
}

.actor-tile.player {
    fill: #2f6f62;
}

.actor-tile.npc {
    fill: #3b5f9f;
}

.movement-edge {
    stroke: #1e2a35;
    stroke-linecap: square;
    stroke-width: 1.6;
}

.wall-edge {
    stroke: #d2385a;
    stroke-linecap: square;
    stroke-width: 3;
}

.details {
    align-self: start;
    background: #ffffff;
    border: 1px solid #d5dbe1;
    padding: 20px;
}

.details p {
    color: #4d5763;
    line-height: 1.45;
    margin-bottom: 18px;
}

dl {
    display: grid;
    gap: 16px;
    margin: 0;
}

dt {
    color: #58606c;
    font-size: 0.78rem;
    font-weight: 800;
    margin-bottom: 4px;
    text-transform: uppercase;
}

dd {
    color: #1f2933;
    font-size: 0.92rem;
    line-height: 1.45;
    margin: 0;
    overflow-wrap: anywhere;
}

@media (max-width: 900px) {
    .viewer-shell {
        padding: 20px;
    }

    .viewer-header {
        align-items: stretch;
        flex-direction: column;
    }

    .control-stack,
    .scenario-tabs,
    .plane-tabs {
        justify-items: start;
        justify-content: flex-start;
    }

    .scenario-layout {
        grid-template-columns: 1fr;
    }

    .viewport {
        min-height: 460px;
        padding: 14px;
    }

    .scene-svg {
        height: 560px;
    }
}
</style>
