<script setup lang="ts">
import { computed, ref } from "vue";
import {
    runScenario,
    scenarioOptions,
    type ScenarioId,
    type ScenarioResult,
    type SceneCoordinate,
} from "./scenarios";

interface RenderTile {
    key: string;
    coordinate: SceneCoordinate;
    actorLabel?: string;
    actorKind?: "NPC" | "Player";
    occupied: boolean;
    blocked: boolean;
    blockerLabel?: string;
}

const selectedScenarioId = ref<ScenarioId>("actor-occupancy");
const scenario = computed<ScenarioResult>(() => runScenario(selectedScenarioId.value));
const rows = computed<RenderTile[][]>(() => buildRows(scenario.value));

function selectScenario(id: ScenarioId): void {
    selectedScenarioId.value = id;
}

function buildRows(result: ScenarioResult): RenderTile[][] {
    const actorTiles = new Map<string, { label: string; kind: "NPC" | "Player" }>();
    const occupiedTiles = new Set(result.occupiedTiles.map(getKey));
    const blockers = new Map(
        result.blockers.map((blocker) => [getKey(blocker.coordinate), blocker.label]),
    );

    for (const actor of result.actors) {
        for (const coordinate of actor.footprint) {
            actorTiles.set(getKey(coordinate), { label: actor.kind, kind: actor.kind });
        }
    }

    const builtRows: RenderTile[][] = [];

    for (let y = result.focus.maxY; y >= result.focus.minY; y -= 1) {
        const row: RenderTile[] = [];

        for (let x = result.focus.minX; x <= result.focus.maxX; x += 1) {
            const coordinate = { x, y, plane: 0 };
            const key = getKey(coordinate);
            const blockerLabel = blockers.get(key);
            const actorTile = actorTiles.get(key);

            row.push({
                key,
                coordinate,
                actorLabel: actorTile?.label,
                actorKind: actorTile?.kind,
                occupied: occupiedTiles.has(key),
                blocked: blockerLabel !== undefined,
                blockerLabel,
            });
        }

        builtRows.push(row);
    }

    return builtRows;
}

function getKey(coordinate: SceneCoordinate): string {
    return `${coordinate.plane}:${coordinate.x}:${coordinate.y}`;
}
</script>

<template>
  <main class="viewer-shell">
    <header class="viewer-header">
      <div>
        <p class="eyebrow">Development viewer</p>
        <h1>OSRS Simulator</h1>
      </div>

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
    </header>

    <section class="scenario-layout" aria-live="polite">
      <div class="viewport" aria-label="Scene tiles">
        <div class="tile-grid">
          <div v-for="row in rows" :key="row[0]?.key" class="tile-row">
            <div
              v-for="tile in row"
              :key="tile.key"
              class="tile"
              :class="{
                occupied: tile.occupied,
                blocked: tile.blocked,
                actor: tile.actorLabel !== undefined,
                'actor-player': tile.actorKind === 'Player',
                'actor-npc': tile.actorKind === 'NPC',
              }"
              :title="`Plane ${tile.coordinate.plane}, x ${tile.coordinate.x}, y ${tile.coordinate.y}`"
            >
              <span v-if="tile.actorLabel" class="tile-label">
                {{ tile.actorLabel }}
              </span>
              <span v-else-if="tile.blockerLabel" class="tile-label">
                {{ tile.blockerLabel }}
              </span>
            </div>
          </div>
        </div>
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
            <dt>Actor Occupancy</dt>
            <dd>
              {{ scenario.occupiedTiles.map((tile) => `${tile.x},${tile.y}`).join(" | ") }}
            </dd>
          </div>
          <div>
            <dt>Scene content</dt>
            <dd>
              <span v-if="scenario.blockers.length === 0">None</span>
              <span v-else>
                {{
                  scenario.blockers
                    .map((blocker) => `${blocker.label} at ${blocker.coordinate.x},${blocker.coordinate.y}`)
                    .join(" | ")
                }}
              </span>
            </dd>
          </div>
        </dl>
      </aside>
    </section>
  </main>
</template>

<style scoped>
.viewer-shell {
    background: #f3f5f0;
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
    max-width: 1180px;
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

h2 {
    font-size: 1.35rem;
    line-height: 1.2;
    margin-bottom: 10px;
}

.scenario-tabs {
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
    max-width: 1180px;
}

.viewport {
    align-content: center;
    background:
        linear-gradient(90deg, rgba(31, 41, 51, 0.05) 1px, transparent 1px),
        linear-gradient(rgba(31, 41, 51, 0.05) 1px, transparent 1px),
        #d9dfd1;
    background-size: 32px 32px;
    min-height: 520px;
    overflow: auto;
    padding: 24px;
}

.tile-grid {
    display: grid;
    gap: 6px;
    justify-content: center;
    min-width: max-content;
}

.tile-row {
    display: flex;
    gap: 6px;
}

.tile {
    align-items: center;
    aspect-ratio: 1;
    background: #eef1ea;
    border: 1px solid #bdc7b7;
    display: flex;
    flex: 0 0 64px;
    justify-content: center;
    position: relative;
}

.tile.blocked {
    background: #64564c;
    border-color: #3f3731;
    color: #ffffff;
}

.tile.occupied {
    box-shadow: inset 0 0 0 4px #d08a1d;
}

.tile.actor {
    color: #ffffff;
}

.tile.actor-player {
    background: #2f6f62;
    border-color: #1d4f45;
}

.tile.actor-npc {
    background: #3b5f9f;
    border-color: #244276;
}

.tile-label {
    font-size: clamp(0.62rem, 0.7rem, 0.78rem);
    font-weight: 800;
    line-height: 1.1;
    max-width: 56px;
    overflow-wrap: anywhere;
    text-align: center;
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

@media (max-width: 860px) {
    .viewer-shell {
        padding: 20px;
    }

    .viewer-header {
        align-items: stretch;
        flex-direction: column;
    }

    .scenario-tabs {
        justify-content: flex-start;
    }

    .scenario-layout {
        grid-template-columns: 1fr;
    }

    .viewport {
        min-height: 380px;
        padding: 16px;
    }

    .tile {
        flex-basis: 52px;
    }
}
</style>
