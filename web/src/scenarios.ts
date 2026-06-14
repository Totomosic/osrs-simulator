import type {
    DevelopmentPlayerChaseScenario,
    Engine,
    EngineModule,
    Scene,
    SceneCoordinate,
    World,
} from "./wasm/EngineModule";
export { loadEngineModule } from "./wasm/EngineModule";

interface ActorSnapshot {
    id: number;
    kind: "Player" | "NPC";
    coordinate: SceneCoordinate;
    size: number;
    speed: number;
    movementTarget:
        | null
        | {
              kind: "SceneCoordinate";
              coordinate: SceneCoordinate;
          }
        | {
              kind: "Actor";
              actorId: number;
              label: string;
          };
}

interface EngineActorSnapshot {
    id: number;
    kind: "Player" | "NPC";
    coordinate: SceneCoordinate;
    size: number;
    speed: number;
    movementTarget:
        | null
        | {
              kind: "SceneCoordinate";
              coordinate?: SceneCoordinate;
              actorId?: number;
          }
        | {
              kind: "Actor";
              coordinate?: SceneCoordinate;
              actorId?: number;
          };
}

export interface RegisteredScenario {
    name: "Player Chase";
    create(module: EngineModule): DevelopmentPlayerChaseScenario;
}

export const registeredScenarios: RegisteredScenario[] = [
    {
        name: "Player Chase",
        create(module) {
            return new PlayerChaseScenario(module);
        },
    },
];

export function createPlayerChaseScenario(
    module: EngineModule,
): DevelopmentPlayerChaseScenario {
    return registeredScenarios[0].create(module);
}

class PlayerChaseScenario implements DevelopmentPlayerChaseScenario {
    private m_Module: EngineModule;
    private m_Engine: Engine;
    private m_World: World;
    private m_Scene: Scene;
    private m_Running = false;
    private m_LastClickBlocked = false;
    private m_PlayerId = 0;
    private m_NpcIds: number[] = [];
    private m_SelectedNpcId = 0;

    public constructor(module: EngineModule) {
        this.m_Module = module;
        this.m_Engine = new module.Engine();
        this.m_World = this.m_Engine.GetWorld();

        const scene = this.m_World.TryGetScene(this.m_World.GetDefaultSceneId());

        if (scene === null) {
            throw new Error("Player Chase requires the default scene.");
        }

        this.m_Scene = scene;
        this.Reset();
    }

    public Step(): void {
        this.m_Engine.Step();
    }

    public ClickSceneCoordinate(x: number, y: number, plane: number): boolean {
        const coordinate = { x, y, plane };

        if (
            !this.m_World.CanPlayerUseSceneCoordinateMovementTarget(
                this.m_PlayerId,
                coordinate,
            )
        ) {
            this.m_LastClickBlocked = true;
            return false;
        }

        this.m_LastClickBlocked = false;
        return this.m_World.SetPlayerSceneCoordinateMovementTarget(
            this.m_PlayerId,
            coordinate,
        );
    }

    public SetRunning(running: boolean): void {
        this.m_Running = running;
    }

    public IsRunning(): boolean {
        return this.m_Running;
    }

    public WasLastClickBlocked(): boolean {
        return this.m_LastClickBlocked;
    }

    public Reset(): void {
        this.removeExistingActors();
        this.m_Scene.RemoveGameObject({ x: 12, y: 4, plane: 0 });

        this.m_Engine = new this.m_Module.Engine();
        this.m_World = this.m_Engine.GetWorld();

        const scene = this.m_World.TryGetScene(this.m_World.GetDefaultSceneId());

        if (scene === null) {
            throw new Error("Player Chase requires the default scene.");
        }

        this.m_Scene = scene;
        this.m_Running = false;
        this.m_LastClickBlocked = false;
        this.m_PlayerId = this.m_World.CreatePlayer(1, 2);
        const npcId = this.m_World.CreateNpc(4, 1);
        this.m_NpcIds = [npcId];
        this.m_SelectedNpcId = npcId;

        const sceneId = this.m_World.GetDefaultSceneId();
        this.m_World.PlaceActor(this.m_PlayerId, sceneId, { x: 8, y: 11, plane: 0 });
        this.m_World.PlaceActor(npcId, sceneId, { x: 18, y: 20, plane: 0 });
        this.m_World.SetActorMovementTarget(npcId, this.m_PlayerId);
        this.m_Scene.PlaceGameObject(
            { x: 12, y: 4, plane: 0 },
            200,
            this.m_Module.CardinalDirection.North,
            3,
            3,
            {
                blocksMovement: true,
                blocksLineOfSight: true,
            },
        );
    }

    public GetSnapshotJson(): string {
        const player = this.readActorSnapshot(this.m_PlayerId);
        const npcs = this.m_NpcIds.map((id) => this.readActorSnapshot(id));
        const selectedNpc =
            npcs.find((npc) => npc.id === this.m_SelectedNpcId) ?? npcs[0];

        return JSON.stringify({
            name: "Player Chase",
            tick: this.GetTick(),
            running: this.m_Running,
            blockedClick: this.m_LastClickBlocked,
            scene: {
                id: this.m_World.GetDefaultSceneId(),
                width: this.GetSceneWidth(),
                height: this.GetSceneHeight(),
                planeCount: this.GetScenePlaneCount(),
            },
            player,
            npc: selectedNpc,
            npcs,
            selectedNpcId: selectedNpc?.id ?? null,
            tiles: this.readFixtureTiles(),
        });
    }

    public GetTick(): number {
        return Number(this.m_Engine.GetCurrentTick());
    }

    public GetSceneWidth(): number {
        return 104;
    }

    public GetSceneHeight(): number {
        return 104;
    }

    public GetScenePlaneCount(): number {
        return 4;
    }

    public GetPlayerId(): number {
        return this.m_PlayerId;
    }

    public GetNpcId(): number {
        return this.m_SelectedNpcId;
    }

    public GetPlayerX(): number {
        return this.readActorSnapshot(this.m_PlayerId).coordinate.x;
    }

    public GetPlayerY(): number {
        return this.readActorSnapshot(this.m_PlayerId).coordinate.y;
    }

    public GetPlayerPlane(): number {
        return this.readActorSnapshot(this.m_PlayerId).coordinate.plane;
    }

    public HasPlayerMovementTarget(): boolean {
        return this.readActorSnapshot(this.m_PlayerId).movementTarget !== null;
    }

    public GetPlayerMovementTargetX(): number {
        return this.getPlayerCoordinateMovementTarget()?.x ?? 0;
    }

    public GetPlayerMovementTargetY(): number {
        return this.getPlayerCoordinateMovementTarget()?.y ?? 0;
    }

    public GetPlayerMovementTargetPlane(): number {
        return this.getPlayerCoordinateMovementTarget()?.plane ?? 0;
    }

    public GetNpcX(): number {
        return this.readActorSnapshot(this.m_SelectedNpcId).coordinate.x;
    }

    public GetNpcY(): number {
        return this.readActorSnapshot(this.m_SelectedNpcId).coordinate.y;
    }

    public GetNpcPlane(): number {
        return this.readActorSnapshot(this.m_SelectedNpcId).coordinate.plane;
    }

    public GetNpcSize(): number {
        return this.readActorSnapshot(this.m_SelectedNpcId).size;
    }

    public HasNpcMovementTarget(): boolean {
        return this.readActorSnapshot(this.m_SelectedNpcId).movementTarget !== null;
    }

    public GetNpcMovementTargetActorId(): number {
        const target = this.readActorSnapshot(this.m_SelectedNpcId).movementTarget;

        return target?.kind === "Actor" ? target.actorId : 0;
    }

    public GetNpcMovementTargetLabel(): string {
        const target = this.readActorSnapshot(this.m_SelectedNpcId).movementTarget;

        return target?.kind === "Actor" ? target.label : "";
    }

    public IsGameObjectTile(x: number, y: number, plane: number): boolean {
        return this.m_Scene.IsGameObjectTile({ x, y, plane });
    }

    public IsPlayerTile(x: number, y: number, plane: number): boolean {
        return this.isActorTile(this.readActorSnapshot(this.m_PlayerId), {
            x,
            y,
            plane,
        });
    }

    public IsNpcTile(x: number, y: number, plane: number): boolean {
        return this.isActorTile(this.readActorSnapshot(this.m_SelectedNpcId), {
            x,
            y,
            plane,
        });
    }

    private readActorSnapshot(actorId: number): ActorSnapshot {
        const snapshotJson = this.m_World.GetActorSnapshot(actorId);

        if (snapshotJson === null) {
            throw new Error(`Actor #${actorId} is not available.`);
        }

        const snapshot = JSON.parse(snapshotJson) as EngineActorSnapshot;

        return {
            ...snapshot,
            movementTarget: this.normalizeMovementTarget(snapshot.movementTarget),
        };
    }

    private normalizeMovementTarget(
        target: EngineActorSnapshot["movementTarget"],
    ): ActorSnapshot["movementTarget"] {
        if (target === null) {
            return null;
        }

        if (target.kind === "SceneCoordinate" && target.coordinate !== undefined) {
            return {
                kind: "SceneCoordinate",
                coordinate: target.coordinate,
            };
        }

        if (target.kind === "Actor" && target.actorId !== undefined) {
            return {
                kind: "Actor",
                actorId: target.actorId,
                label:
                    target.actorId === this.m_PlayerId
                        ? `Player #${this.m_PlayerId}`
                        : `Actor #${target.actorId}`,
            };
        }

        return null;
    }

    private readFixtureTiles(): {
        coordinate: SceneCoordinate;
        flags: string[];
        gameObject?: {
            id: number;
            origin: SceneCoordinate;
            sizeX: number;
            sizeY: number;
        };
    }[] {
        const tiles = [];

        for (let x = 12; x <= 14; x += 1) {
            for (let y = 4; y <= 6; y += 1) {
                const coordinate = { x, y, plane: 0 };
                tiles.push({
                    coordinate,
                    flags: this.m_Scene.GetTileFlagLabels(coordinate),
                    gameObject: this.m_Scene.IsGameObjectTile(coordinate)
                        ? {
                              id: 200,
                              origin: { x: 12, y: 4, plane: 0 },
                              sizeX: 3,
                              sizeY: 3,
                          }
                        : undefined,
                });
            }
        }

        return tiles;
    }

    private getPlayerCoordinateMovementTarget(): SceneCoordinate | null {
        const target = this.readActorSnapshot(this.m_PlayerId).movementTarget;

        return target?.kind === "SceneCoordinate" ? target.coordinate : null;
    }

    private isActorTile(actor: ActorSnapshot, coordinate: SceneCoordinate): boolean {
        return (
            coordinate.plane === actor.coordinate.plane &&
            coordinate.x >= actor.coordinate.x &&
            coordinate.x < actor.coordinate.x + actor.size &&
            coordinate.y >= actor.coordinate.y &&
            coordinate.y < actor.coordinate.y + actor.size
        );
    }

    private removeExistingActors(): void {
        if (this.m_PlayerId !== 0) {
            this.m_World.RemoveActor(this.m_PlayerId);
        }

        for (const npcId of this.m_NpcIds) {
            this.m_World.RemoveActor(npcId);
        }
    }
}
