<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref, shallowRef } from "vue";
import { loadEngineModule } from "./wasm/EngineModule";
import type { EngineModule, RecordingPlayback } from "./wasm/EngineModule";

interface RecordingActor {
    id: number;
    kind: "Player" | "Npc";
    playerIndex?: number;
    npcIndex?: number;
    present: boolean;
    sceneMembership: {
        sceneId: number;
        coordinate: {
            x: number;
            y: number;
            plane: number;
        };
    };
    size: number;
    speed: number;
    combatComposition: {
        stats: Record<string, number>;
        baseStats: Record<string, number>;
        bonuses: Record<string, number>;
        attackType: string;
        magicBaseMaximumHit: number;
        weapon: {
            id: number;
            range: number;
            speed: number;
            projectileId: number;
        };
        equipmentProvenance?: Array<{
            slot: string;
            pieceId: number;
        }>;
    };
    debug: {
        movementTarget:
            | null
            | { kind: "Actor"; actorId: number }
            | {
                  kind: "SceneCoordinate";
                  coordinate: { x: number; y: number; plane: number };
              };
        attackTimer: number;
    };
}

interface RecordingSceneEntity {
    kind: "GameObject" | "WallObject";
    sceneId: number;
    id: number;
    coordinate: {
        x: number;
        y: number;
        plane: number;
    };
    direction: "North" | "East" | "South" | "West";
    sizeX?: number;
    sizeY?: number;
    collision: {
        blocksMovement: boolean;
        blocksLineOfSight: boolean;
    };
}

interface RecordingQueuedDamageEvent {
    id: number;
    attackId: number;
    targetId: number;
    damage: number;
    delayTicks: number;
}

interface RecordingAttack {
    id: number;
    tick: number;
    attackerId: number;
    targetId: number;
    callback: string;
    queuedDamageEvents: RecordingQueuedDamageEvent[];
    projectile?: {
        projectileId: number;
        targetActorId: number;
    };
}

interface RecordingDamageApplication {
    damageEventId: number;
    attackId: number;
    tick: number;
    targetId: number;
    queuedDamage: number;
    appliedDamage: number;
}

interface RecordingProjectile {
    projectileId: number;
    targetActorId: number;
    lastKnownTargetCenter: {
        x: number;
        y: number;
        plane: number;
    };
    elapsedTicks: number;
    totalTicks: number;
}

const bundledSamples = [
    {
        label: "Minimal Encounter Recording",
        path: "recordings/minimal-encounter-recording.json",
    },
];

const engineModule = shallowRef<EngineModule | null>(null);
const playback = shallowRef<RecordingPlayback | null>(null);
const selectedSamplePath = ref(bundledSamples[0].path);
const selectedActorId = ref<number | null>(null);
const loadError = ref("");
const actorJsonRevision = ref(0);

const encounterName = computed(
    () => playback.value?.GetEncounterName() ?? "No recording loaded",
);
const secondsPerTick = computed(() => playback.value?.GetSecondsPerTick() ?? 0);
const currentTick = computed(() => playback.value?.GetCurrentTick() ?? 0);
const initialTick = computed(() => playback.value?.GetInitialTick() ?? 0);
const lastTick = computed(() => playback.value?.GetLastTick() ?? 0);
const hasPlayback = computed(() => playback.value !== null);
const actors = computed<RecordingActor[]>(() => {
    void actorJsonRevision.value;

    if (playback.value === null) {
        return [];
    }

    return JSON.parse(playback.value.GetActorsJson()) as RecordingActor[];
});
const sceneEntities = computed<RecordingSceneEntity[]>(() => {
    void actorJsonRevision.value;

    if (playback.value === null) {
        return [];
    }

    return JSON.parse(playback.value.GetSceneEntitiesJson()) as RecordingSceneEntity[];
});
const attacks = computed<RecordingAttack[]>(() => {
    void actorJsonRevision.value;

    if (playback.value === null) {
        return [];
    }

    return JSON.parse(playback.value.GetAttacksJson()) as RecordingAttack[];
});
const damageApplications = computed<RecordingDamageApplication[]>(() => {
    void actorJsonRevision.value;

    if (playback.value === null) {
        return [];
    }

    return JSON.parse(
        playback.value.GetDamageApplicationsJson(),
    ) as RecordingDamageApplication[];
});
const projectiles = computed<RecordingProjectile[]>(() => {
    void actorJsonRevision.value;

    if (playback.value === null) {
        return [];
    }

    return JSON.parse(playback.value.GetProjectilesJson()) as RecordingProjectile[];
});
const selectedActor = computed(
    () => actors.value.find((actor) => actor.id === selectedActorId.value) ?? null,
);
const visibleSceneBounds = computed(() => {
    if (actors.value.length === 0 && sceneEntities.value.length === 0) {
        return { minX: 0, maxX: 4, minY: 0, maxY: 4 };
    }

    const xs = [
        ...actors.value.map((actor) => actor.sceneMembership.coordinate.x),
        ...sceneEntities.value.map((sceneEntity) => sceneEntity.coordinate.x),
        ...projectiles.value.map((projectile) =>
            Math.floor(projectile.lastKnownTargetCenter.x),
        ),
    ];
    const ys = [
        ...actors.value.map((actor) => actor.sceneMembership.coordinate.y),
        ...sceneEntities.value.map((sceneEntity) => sceneEntity.coordinate.y),
        ...projectiles.value.map((projectile) =>
            Math.floor(projectile.lastKnownTargetCenter.y),
        ),
    ];

    return {
        minX: Math.min(...xs) - 2,
        maxX: Math.max(...xs) + 2,
        minY: Math.min(...ys) - 2,
        maxY: Math.max(...ys) + 2,
    };
});
const sceneRows = computed(() => {
    const rows = [];

    for (let y = visibleSceneBounds.value.maxY; y >= visibleSceneBounds.value.minY; --y) {
        const cells = [];

        for (let x = visibleSceneBounds.value.minX; x <= visibleSceneBounds.value.maxX; ++x) {
            cells.push({ x, y });
        }

        rows.push(cells);
    }

    return rows;
});

function actorLabel(actor: RecordingActor): string {
    const index =
        actor.kind === "Player" ? actor.playerIndex ?? 0 : actor.npcIndex ?? 0;
    return `${actor.kind} ${index} (#${actor.id})`;
}

function actorAtCell(x: number, y: number): RecordingActor | null {
    return (
        actors.value.find(
            (actor) =>
                actor.sceneMembership.coordinate.x === x &&
                actor.sceneMembership.coordinate.y === y,
        ) ?? null
    );
}

function sceneEntityAtCell(x: number, y: number): RecordingSceneEntity | null {
    return (
        sceneEntities.value.find(
            (sceneEntity) =>
                sceneEntity.coordinate.x === x && sceneEntity.coordinate.y === y,
        ) ?? null
    );
}

function projectileAtCell(x: number, y: number): RecordingProjectile | null {
    return (
        projectiles.value.find(
            (projectile) =>
                Math.floor(projectile.lastKnownTargetCenter.x) === x &&
                Math.floor(projectile.lastKnownTargetCenter.y) === y,
        ) ?? null
    );
}

function actorName(actorId: number): string {
    const actor = actors.value.find((candidate) => candidate.id === actorId);
    return actor === undefined ? `Actor #${actorId}` : actorLabel(actor);
}

function equipmentProvenanceText(actor: RecordingActor): string {
    const provenance = actor.combatComposition.equipmentProvenance ?? [];

    if (provenance.length === 0) {
        return "None";
    }

    return provenance
        .map((piece) => `${piece.slot} #${piece.pieceId}`)
        .join(", ");
}

function refreshActors(): void {
    actorJsonRevision.value += 1;

    if (
        selectedActorId.value === null ||
        !actors.value.some((actor) => actor.id === selectedActorId.value)
    ) {
        selectedActorId.value = actors.value[0]?.id ?? null;
    }
}

function formatRecordingLoadError(error: unknown): string {
    const message = error instanceof Error ? error.message : "";

    if (message.length > 0 && message !== "Aborted(undefined)") {
        return `Recording load failed: ${message}`;
    }

    return "Recording load failed: invalid or unsupported recording JSON.";
}

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
        refreshActors();
    } catch (error) {
        playback.value = null;
        selectedActorId.value = null;
        refreshActors();
        loadError.value = formatRecordingLoadError(error);
    }
}

function previousTick(): void {
    playback.value?.PreviousTick();
    refreshActors();
}

function nextTick(): void {
    playback.value?.NextTick();
    refreshActors();
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

    <section class="recording-inspector" aria-label="Actor inspector">
      <div class="recording-scene" aria-label="Recording scene">
        <div v-for="row in sceneRows" :key="row[0].y" class="scene-row">
          <button
            v-for="cell in row"
            :key="`${cell.x},${cell.y}`"
            type="button"
            class="scene-cell"
            :class="{ selected: actorAtCell(cell.x, cell.y)?.id === selectedActorId }"
            :aria-label="`Tile ${cell.x}, ${cell.y}`"
            @click="selectedActorId = actorAtCell(cell.x, cell.y)?.id ?? selectedActorId"
          >
            <span v-if="actorAtCell(cell.x, cell.y)" class="actor-token">
              {{ actorAtCell(cell.x, cell.y)?.kind === "Player" ? "P" : "N" }}
            </span>
            <span
              v-if="projectileAtCell(cell.x, cell.y)"
              class="projectile-token"
              :title="`Projectile #${projectileAtCell(cell.x, cell.y)?.projectileId}`"
            >
              *
            </span>
            <span
              v-if="!actorAtCell(cell.x, cell.y) && sceneEntityAtCell(cell.x, cell.y)"
              class="scene-entity-token"
              :title="`${sceneEntityAtCell(cell.x, cell.y)?.kind} #${sceneEntityAtCell(cell.x, cell.y)?.id}`"
            >
              {{ sceneEntityAtCell(cell.x, cell.y)?.kind === "GameObject" ? "G" : "W" }}
            </span>
          </button>
        </div>
      </div>

      <div class="actor-panel">
        <label>
          Actor
          <select v-model.number="selectedActorId" :disabled="actors.length === 0">
            <option
              v-for="actor in actors"
              :key="actor.id"
              :value="actor.id"
            >
              {{ actorLabel(actor) }}
            </option>
          </select>
        </label>

        <dl v-if="selectedActor" class="actor-stats">
          <div>
            <dt>Hitpoints</dt>
            <dd>
              {{ selectedActor.combatComposition.stats.hitpoints }} /
              {{ selectedActor.combatComposition.baseStats.hitpoints }}
            </dd>
          </div>
          <div>
            <dt>Scene</dt>
            <dd>
              {{ selectedActor.sceneMembership.sceneId }}:
              {{ selectedActor.sceneMembership.coordinate.x }},
              {{ selectedActor.sceneMembership.coordinate.y }},
              {{ selectedActor.sceneMembership.coordinate.plane }}
            </dd>
          </div>
          <div>
            <dt>Stats</dt>
            <dd>
              Atk {{ selectedActor.combatComposition.stats.attack }},
              Str {{ selectedActor.combatComposition.stats.strength }},
              Def {{ selectedActor.combatComposition.stats.defence }},
              Rng {{ selectedActor.combatComposition.stats.ranged }},
              Mag {{ selectedActor.combatComposition.stats.magic }}
            </dd>
          </div>
          <div>
            <dt>Combat Composition</dt>
            <dd>
              {{ selectedActor.combatComposition.attackType }},
              weapon #{{ selectedActor.combatComposition.weapon.id }},
              range {{ selectedActor.combatComposition.weapon.range }},
              speed {{ selectedActor.combatComposition.weapon.speed }}
            </dd>
          </div>
          <div>
            <dt>Equipment Provenance</dt>
            <dd>{{ equipmentProvenanceText(selectedActor) }}</dd>
          </div>
          <div>
            <dt>Movement Target</dt>
            <dd>
              {{ selectedActor.debug.movementTarget?.kind ?? "None" }}
            </dd>
          </div>
          <div>
            <dt>Attack Timer</dt>
            <dd>{{ selectedActor.debug.attackTimer }}</dd>
          </div>
        </dl>
      </div>
    </section>

    <section class="recording-events" aria-label="Current Tick events">
      <h2>Current Tick Events</h2>
      <ol>
        <li v-for="attack in attacks" :key="`attack-${attack.id}`">
          Attack #{{ attack.id }}:
          {{ actorName(attack.attackerId) }} -> {{ actorName(attack.targetId) }},
          {{ attack.callback }},
          damage event #{{ attack.queuedDamageEvents[0]?.id ?? "n/a" }}
        </li>
        <li
          v-for="damageApplication in damageApplications"
          :key="`damage-${damageApplication.damageEventId}`"
        >
          Damage #{{ damageApplication.damageEventId }}:
          attack #{{ damageApplication.attackId }},
          {{ actorName(damageApplication.targetId) }},
          {{ damageApplication.appliedDamage }}/{{ damageApplication.queuedDamage }}
        </li>
      </ol>
      <p v-if="attacks.length === 0 && damageApplications.length === 0">
        No combat events.
      </p>
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

.recording-inspector {
    display: grid;
    gap: 18px;
    grid-template-columns: minmax(260px, 1fr) minmax(260px, 320px);
}

.recording-scene {
    border: 1px solid #9cadb7;
    display: inline-grid;
    gap: 2px;
    padding: 8px;
    width: max-content;
}

.scene-row {
    display: flex;
    gap: 2px;
}

.scene-cell {
    align-items: center;
    background: #f6f8f9;
    border: 1px solid #d7e0e5;
    color: #1e252b;
    display: flex;
    font: inherit;
    font-size: 0.8rem;
    font-weight: 900;
    height: 32px;
    justify-content: center;
    padding: 0;
    position: relative;
    width: 32px;
}

.scene-cell.selected {
    border-color: #1c5d7a;
    box-shadow: inset 0 0 0 2px #1c5d7a;
}

.actor-token {
    align-items: center;
    background: #2e6f55;
    border-radius: 999px;
    color: white;
    display: flex;
    height: 20px;
    justify-content: center;
    width: 20px;
}

.scene-entity-token {
    align-items: center;
    background: #7b5c2e;
    border-radius: 4px;
    color: white;
    display: flex;
    font-size: 0.7rem;
    height: 18px;
    justify-content: center;
    width: 18px;
}

.projectile-token {
    align-items: center;
    background: #c43c35;
    border-radius: 999px;
    color: white;
    display: flex;
    font-size: 0.78rem;
    font-weight: 900;
    height: 14px;
    justify-content: center;
    position: absolute;
    right: 2px;
    top: 2px;
    width: 14px;
}

.actor-panel {
    display: grid;
    gap: 14px;
}

.actor-panel label {
    display: grid;
    font-weight: 900;
    gap: 6px;
}

.actor-panel select {
    border: 1px solid #8aa0ad;
    border-radius: 6px;
    font: inherit;
    min-height: 36px;
    padding: 6px 10px;
}

.actor-stats {
    display: grid;
    gap: 10px;
    margin: 0;
}

.actor-stats div {
    border-left: 3px solid #2e6f55;
    padding-left: 10px;
}

.actor-stats dt {
    color: #52616a;
    font-size: 0.78rem;
    font-weight: 900;
    text-transform: uppercase;
}

.actor-stats dd {
    font-weight: 800;
    line-height: 1.35;
    margin: 2px 0 0;
}

.recording-events {
    display: grid;
    gap: 8px;
}

.recording-events h2 {
    font-size: 1rem;
    margin: 0;
}

.recording-events ol {
    display: grid;
    gap: 6px;
    margin: 0;
    padding-left: 20px;
}

@media (max-width: 760px) {
    .recording-inspector {
        grid-template-columns: 1fr;
    }

    .recording-scene {
        max-width: 100%;
        overflow: auto;
    }
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
