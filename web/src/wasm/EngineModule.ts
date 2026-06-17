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

export interface WeaponDefinition {
    id: number;
    range: number;
    speed: number;
}

export interface CombatStats {
    attack: number;
    strength: number;
    defence: number;
    ranged: number;
    magic: number;
    hitpoints: number;
}

export interface EquipmentBonuses {
    stabAttack: number;
    slashAttack: number;
    crushAttack: number;
    magicAttack: number;
    rangedAttack: number;
    stabDefence: number;
    slashDefence: number;
    crushDefence: number;
    magicDefence: number;
    rangedDefenceLight: number;
    rangedDefenceStandard: number;
    rangedDefenceHeavy: number;
    meleeStrength: number;
    rangedStrength: number;
    magicDamagePercent: number;
}

export interface StyleBonus {
    attack: number;
    strength: number;
    defence: number;
    ranged: number;
    magic: number;
}

export interface DpsRequest {
    attackerStats: CombatStats;
    defenderStats: CombatStats;
    attackerBonuses: EquipmentBonuses;
    defenderBonuses: EquipmentBonuses;
    attackerStyle: StyleBonus;
    defenderStyle: StyleBonus;
    attackType: AttackType;
    weaponSpeedTicks: number;
    attackPrayerMultiplier: number;
    strengthPrayerMultiplier: number;
    defencePrayerMultiplier: number;
    attackLevelMultiplier: number;
    strengthLevelMultiplier: number;
    defenceLevelMultiplier: number;
    finalAttackRollMultiplier: number;
    finalDefenceRollMultiplier: number;
    finalDamageMultiplier: number;
}

export interface DpsResult {
    attackRoll: number;
    defenceRoll: number;
    maximumHit: number;
    hitChance: number;
    expectedDamagePerAttack: number;
    secondsPerAttack: number;
    dps: number;
}

export interface DpsService {
    CalculateExpected(request: DpsRequest): DpsResult;
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
    SetActorWeaponDefinition(
        actorId: number,
        weaponDefinition: WeaponDefinition,
    ): boolean;
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
export type AttackType = number | "Stab" | "Slash" | "Crush";

export interface EngineModule {
    Engine: new () => Engine;
    DpsService: new () => DpsService;
    CardinalDirection: {
        North: CardinalDirection;
        East?: CardinalDirection;
        South?: CardinalDirection;
        West?: CardinalDirection;
    };
    AttackType: {
        Stab: AttackType;
        Slash: AttackType;
        Crush: AttackType;
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
