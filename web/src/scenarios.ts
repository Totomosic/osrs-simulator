import type {
    ActionFeedback,
    CardinalDirection,
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

interface GameObjectSnapshot {
    id: number;
    origin: SceneCoordinate;
    sizeX: number;
    sizeY: number;
    footprintWidth: number;
    footprintHeight: number;
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
    private m_ActionFeedback: ActionFeedback = { state: "none" };
    private m_PlayerId = 0;
    private m_NpcIds: number[] = [];
    private m_SelectedNpcId = 0;
    private m_GameObjects: GameObjectSnapshot[] = [];
    private m_NextGameObjectId = 201;

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
            this.m_ActionFeedback = { state: "blocked-movement" };
            return false;
        }

        this.m_LastClickBlocked = false;
        const moved = this.m_World.SetPlayerSceneCoordinateMovementTarget(
            this.m_PlayerId,
            coordinate,
        );

        this.m_ActionFeedback = moved
            ? { state: "none" }
            : { state: "blocked-movement" };
        return moved;
    }

    public PlaceNpc(
        size: number,
        speed: number,
        x: number,
        y: number,
        plane: number,
    ): boolean {
        const npcId = this.m_World.CreateNpc(size, speed);
        const placed = this.m_World.PlaceActor(
            npcId,
            this.m_World.GetDefaultSceneId(),
            { x, y, plane },
        );

        if (!placed) {
            this.m_World.RemoveActor(npcId);
            this.m_ActionFeedback = { state: "placement-failure" };
            return false;
        }

        this.m_World.SetActorMovementTarget(npcId, this.m_PlayerId);
        this.m_NpcIds.push(npcId);
        this.m_SelectedNpcId = npcId;
        this.m_LastClickBlocked = false;
        this.m_ActionFeedback = { state: "none" };
        return true;
    }

    public RemoveNpc(x: number, y: number, plane: number): boolean {
        const coordinate = { x, y, plane };
        const npcId = this.findNpcIdAtCoordinate(coordinate);

        if (npcId === null) {
            this.m_ActionFeedback = { state: "removal-failure" };
            return false;
        }

        this.m_World.RemoveActor(npcId);
        this.m_NpcIds = this.m_NpcIds.filter((id) => id !== npcId);

        if (this.m_SelectedNpcId === npcId) {
            this.m_SelectedNpcId = this.m_NpcIds[0] ?? 0;
        }

        this.m_LastClickBlocked = false;
        this.m_ActionFeedback = { state: "none" };
        return true;
    }

    public PlaceGameObject(
        x: number,
        y: number,
        plane: number,
        sizeX: number,
        sizeY: number,
        direction: CardinalDirection,
        blocksMovement: boolean,
        blocksLineOfSight: boolean,
    ): boolean {
        const coordinate = { x, y, plane };
        const id = this.m_NextGameObjectId;
        const normalizedDirection = this.normalizeCardinalDirection(direction);
        const placed = this.m_Scene.PlaceGameObject(
            coordinate,
            id,
            normalizedDirection,
            sizeX,
            sizeY,
            {
                blocksMovement,
                blocksLineOfSight,
            },
        );

        if (!placed) {
            this.m_ActionFeedback = { state: "placement-failure" };
            return false;
        }

        this.m_NextGameObjectId += 1;
        this.m_GameObjects.push({
            id,
            origin: coordinate,
            sizeX,
            sizeY,
            footprintWidth: this.getGameObjectFootprintWidth(
                direction,
                sizeX,
                sizeY,
            ),
            footprintHeight: this.getGameObjectFootprintHeight(
                direction,
                sizeX,
                sizeY,
            ),
        });
        this.m_LastClickBlocked = false;
        this.m_ActionFeedback = { state: "none" };
        return true;
    }

    public RemoveGameObject(x: number, y: number, plane: number): boolean {
        const gameObject = this.findGameObjectAtCoordinate({ x, y, plane });

        if (gameObject === null) {
            this.m_ActionFeedback = { state: "removal-failure" };
            return false;
        }

        const removed = this.m_Scene.RemoveGameObject({ x, y, plane });

        if (!removed) {
            this.m_ActionFeedback = { state: "removal-failure" };
            return false;
        }

        this.m_GameObjects = this.m_GameObjects.filter(
            (candidate) => candidate.id !== gameObject.id,
        );
        this.m_LastClickBlocked = false;
        this.m_ActionFeedback = { state: "none" };
        return true;
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
        this.m_ActionFeedback = { state: "none" };
        this.m_PlayerId = this.m_World.CreatePlayer(1, 2);
        const npcId = this.m_World.CreateNpc(4, 1);
        this.m_NpcIds = [npcId];
        this.m_SelectedNpcId = npcId;
        this.m_GameObjects = [];
        this.m_NextGameObjectId = 201;

        const sceneId = this.m_World.GetDefaultSceneId();
        this.m_World.PlaceActor(this.m_PlayerId, sceneId, { x: 8, y: 11, plane: 0 });
        this.m_World.PlaceActor(npcId, sceneId, { x: 18, y: 20, plane: 0 });
        this.m_World.SetActorMovementTarget(npcId, this.m_PlayerId);
        const placed = this.m_Scene.PlaceGameObject(
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

        if (placed) {
            this.m_GameObjects.push({
                id: 200,
                origin: { x: 12, y: 4, plane: 0 },
                sizeX: 3,
                sizeY: 3,
                footprintWidth: 3,
                footprintHeight: 3,
            });
        }
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
            actionFeedback: this.m_ActionFeedback,
            scene: {
                id: this.m_World.GetDefaultSceneId(),
                width: this.GetSceneWidth(),
                height: this.GetSceneHeight(),
                planeCount: this.GetScenePlaneCount(),
            },
            player,
            npc: selectedNpc ?? null,
            npcs,
            selectedNpcId: selectedNpc?.id ?? null,
            selectedNpc: selectedNpc ?? null,
            tiles: this.readScenarioTiles(),
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
        return this.getSelectedNpcSnapshot()?.coordinate.x ?? 0;
    }

    public GetNpcY(): number {
        return this.getSelectedNpcSnapshot()?.coordinate.y ?? 0;
    }

    public GetNpcPlane(): number {
        return this.getSelectedNpcSnapshot()?.coordinate.plane ?? 0;
    }

    public GetNpcSize(): number {
        return this.getSelectedNpcSnapshot()?.size ?? 0;
    }

    public HasNpcMovementTarget(): boolean {
        return (this.getSelectedNpcSnapshot()?.movementTarget ?? null) !== null;
    }

    public GetNpcMovementTargetActorId(): number {
        const target = this.getSelectedNpcSnapshot()?.movementTarget;

        return target?.kind === "Actor" ? target.actorId : 0;
    }

    public GetNpcMovementTargetLabel(): string {
        const target = this.getSelectedNpcSnapshot()?.movementTarget;

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
        const selectedNpc = this.getSelectedNpcSnapshot();

        return selectedNpc === null
            ? false
            : this.isActorTile(selectedNpc, {
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

    private readScenarioTiles(): {
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

        for (const gameObject of this.m_GameObjects) {
            for (
                let x = gameObject.origin.x;
                x < gameObject.origin.x + gameObject.footprintWidth;
                x += 1
            ) {
                for (
                    let y = gameObject.origin.y;
                    y < gameObject.origin.y + gameObject.footprintHeight;
                    y += 1
                ) {
                    const coordinate = { x, y, plane: gameObject.origin.plane };
                    tiles.push({
                        coordinate,
                        flags: this.m_Scene.GetTileFlagLabels(coordinate),
                        gameObject: this.m_Scene.IsGameObjectTile(coordinate)
                            ? {
                                  id: gameObject.id,
                                  origin: gameObject.origin,
                                  sizeX: gameObject.sizeX,
                                  sizeY: gameObject.sizeY,
                              }
                            : undefined,
                    });
                }
            }
        }

        return tiles;
    }

    private getPlayerCoordinateMovementTarget(): SceneCoordinate | null {
        const target = this.readActorSnapshot(this.m_PlayerId).movementTarget;

        return target?.kind === "SceneCoordinate" ? target.coordinate : null;
    }

    private getSelectedNpcSnapshot(): ActorSnapshot | null {
        return this.m_SelectedNpcId === 0
            ? null
            : this.readActorSnapshot(this.m_SelectedNpcId);
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

    private findNpcIdAtCoordinate(coordinate: SceneCoordinate): number | null {
        if (
            this.m_SelectedNpcId !== 0 &&
            this.isActorTile(
                this.readActorSnapshot(this.m_SelectedNpcId),
                coordinate,
            )
        ) {
            return this.m_SelectedNpcId;
        }

        for (let index = this.m_NpcIds.length - 1; index >= 0; index -= 1) {
            const npcId = this.m_NpcIds[index];

            if (this.isActorTile(this.readActorSnapshot(npcId), coordinate)) {
                return npcId;
            }
        }

        return null;
    }

    private findGameObjectAtCoordinate(
        coordinate: SceneCoordinate,
    ): GameObjectSnapshot | null {
        return (
            this.m_GameObjects.find((gameObject) =>
                this.isGameObjectTile(gameObject, coordinate),
            ) ?? null
        );
    }

    private isGameObjectTile(
        gameObject: GameObjectSnapshot,
        coordinate: SceneCoordinate,
    ): boolean {
        return (
            coordinate.plane === gameObject.origin.plane &&
            coordinate.x >= gameObject.origin.x &&
            coordinate.x < gameObject.origin.x + gameObject.footprintWidth &&
            coordinate.y >= gameObject.origin.y &&
            coordinate.y < gameObject.origin.y + gameObject.footprintHeight
        );
    }

    private normalizeCardinalDirection(
        direction: CardinalDirection,
    ): CardinalDirection {
        if (typeof direction === "number") {
            return direction;
        }

        return this.m_Module.CardinalDirection[direction] ?? direction;
    }

    private getGameObjectFootprintWidth(
        direction: CardinalDirection,
        sizeX: number,
        sizeY: number,
    ): number {
        const label = this.getCardinalDirectionLabel(direction);

        return label === "East" || label === "West" ? sizeY : sizeX;
    }

    private getGameObjectFootprintHeight(
        direction: CardinalDirection,
        sizeX: number,
        sizeY: number,
    ): number {
        const label = this.getCardinalDirectionLabel(direction);

        return label === "East" || label === "West" ? sizeX : sizeY;
    }

    private getCardinalDirectionLabel(
        direction: CardinalDirection,
    ): "North" | "East" | "South" | "West" {
        if (direction === this.m_Module.CardinalDirection.East || direction === "East") {
            return "East";
        }

        if (
            direction === this.m_Module.CardinalDirection.South ||
            direction === "South"
        ) {
            return "South";
        }

        if (direction === this.m_Module.CardinalDirection.West || direction === "West") {
            return "West";
        }

        return "North";
    }
}
