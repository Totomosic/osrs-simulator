export interface EngineModuleOptions {
    locateFile(path: string): string;
}

export interface EngineModuleFactory {
    (options: EngineModuleOptions): Promise<EngineModule>;
}

export interface Engine {
    Step(): void;
    GetCurrentTick(): number;
    GetWorld(): World;
}

export interface SceneCoordinate {
    x: number;
    y: number;
    plane: number;
}

export interface CollisionProfile {
    blocksMovement: boolean;
    blocksLineOfSight: boolean;
}

export interface ActionFeedback {
    state: "none" | "blocked-movement" | "placement-failure" | "removal-failure";
}

export interface Scene {
    PlaceGameObject(
        coordinate: SceneCoordinate,
        id: number,
        direction: CardinalDirection,
        sizeX: number,
        sizeY: number,
        collisionProfile: CollisionProfile,
    ): boolean;
    RemoveGameObject(coordinate: SceneCoordinate): boolean;
    IsGameObjectTile(coordinate: SceneCoordinate): boolean;
    HasLineOfSight(
        sourceAnchor: SceneCoordinate,
        sourceActorSize: number,
        target: SceneCoordinate,
        range: number,
    ): boolean;
    HasActorLineOfSight(
        sourceAnchor: SceneCoordinate,
        sourceActorSize: number,
        targetAnchor: SceneCoordinate,
        targetActorSize: number,
        range: number,
    ): boolean;
    GetTileFlagLabels(coordinate: SceneCoordinate): string[];
}

export interface World {
    GetDefaultSceneId(): number;
    TryGetScene(sceneId: number): Scene | null;
    CreatePlayer(size: number, speed: number): number;
    CreateNpc(size: number, speed: number): number;
    PlaceActor(actorId: number, sceneId: number, coordinate: SceneCoordinate): boolean;
    RemoveActor(actorId: number): boolean;
    SetActorMovementTarget(actorId: number, targetActorId: number): boolean;
    SetPlayerSceneCoordinateMovementTarget(
        actorId: number,
        coordinate: SceneCoordinate,
    ): boolean;
    CanPlayerUseSceneCoordinateMovementTarget(
        actorId: number,
        coordinate: SceneCoordinate,
    ): boolean;
    GetActorSnapshot(actorId: number): string | null;
}

export type CardinalDirection = number | "North" | "East" | "South" | "West";

export interface EngineModule {
    Engine: new () => Engine;
    CardinalDirection: {
        North: CardinalDirection;
        East?: CardinalDirection;
        South?: CardinalDirection;
        West?: CardinalDirection;
    };
    World?: new () => World;
}

const generatedModuleUrl = new URL(
    "./generated/EngineModule.js",
    import.meta.url,
).href;
const generatedWasmUrl = new URL(
    "./generated/EngineModule.wasm",
    import.meta.url,
).href;

export function loadEngineModule(
    factory?: EngineModuleFactory,
): Promise<EngineModule> {
    if (factory !== undefined) {
        return createEngineModule(factory);
    }

    return import(/* @vite-ignore */ generatedModuleUrl).then((generatedModule) =>
        createEngineModule(generatedModule.default as EngineModuleFactory),
    );
}

function createEngineModule(factory: EngineModuleFactory): Promise<EngineModule> {
    return factory({
        locateFile: (path) =>
            path === "EngineModule.wasm" ? generatedWasmUrl : path,
    });
}
