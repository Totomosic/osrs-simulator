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

export interface DevelopmentPlayerChaseScenario {
    Step(): void;
    Reset(): void;
    ClickSceneCoordinate(x: number, y: number, plane: number): boolean;
    PlaceNpc(size: number, speed: number, x: number, y: number, plane: number): boolean;
    RemoveNpc(x: number, y: number, plane: number): boolean;
    PlaceGameObject(
        x: number,
        y: number,
        plane: number,
        sizeX: number,
        sizeY: number,
        direction: CardinalDirection,
        blocksMovement: boolean,
        blocksLineOfSight: boolean,
    ): boolean;
    RemoveGameObject(x: number, y: number, plane: number): boolean;
    HasLineOfSight(
        actorId: number,
        x: number,
        y: number,
        plane: number,
        range: number,
    ): boolean;
    HasActorLineOfSight(
        sourceActorId: number,
        targetActorId: number,
        range: number,
    ): boolean;
    SetRunning(running: boolean): void;
    IsRunning(): boolean;
    WasLastClickBlocked(): boolean;
    GetCurrentTick(): number | bigint;
    GetWorld(): World;
    GetPlayerId(): number | bigint;
    GetNpcIdsJson(): string;
    GetSelectedNpcId(): number | bigint;
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
    id: number | bigint;
    range: number;
    speed: number;
    projectileId?: number;
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

export interface AttackComposition {
    attackType: AttackType;
    stats: CombatStats;
    bonuses: EquipmentBonuses;
    weapon: WeaponDefinition;
}

export interface CombatComposition {
    stats: CombatStats;
    baseStats: CombatStats;
    bonuses: EquipmentBonuses;
    attackType: AttackType;
    magicBaseMaximumHit: number;
    weapon: WeaponDefinition;
}

export interface DefenceComposition {
    stats: CombatStats;
    bonuses: EquipmentBonuses;
}

export interface EquipmentPiece {
    id: number;
    name: string;
    slot: EquipmentSlot;
    bonuses: EquipmentBonuses;
    hasWeapon: boolean;
    weaponId: number | bigint;
}

export interface EquipmentPieceVector {
    size(): number;
    get(index: number): EquipmentPiece | undefined;
    delete(): void;
}

export interface CombatCompositionRecord {
    id: number | bigint;
    name: string;
    source: CombatCompositionSource;
    composition: CombatComposition;
}

export interface CombatCompositionRecordVector {
    size(): number;
    get(index: number): CombatCompositionRecord | undefined;
    delete(): void;
}

export interface NpcDefinition {
    id: bigint;
    name: string;
    hasCombatLevel: boolean;
    combatLevel: number;
    size: number;
    speed: number;
    combatCompositionId: bigint;
}

export interface NpcDefinitionVector {
    size(): number;
    get(index: number): NpcDefinition | undefined;
    delete(): void;
}

export interface DpsRequest {
    attackComposition: AttackComposition;
    defenceComposition: DefenceComposition;
    attackerStyle: StyleBonus;
    defenderStyle: StyleBonus;
    defenderKind: DefenderKind;
    attackPrayerMultiplier: number;
    strengthPrayerMultiplier: number;
    defencePrayerMultiplier: number;
    attackLevelMultiplier: number;
    strengthLevelMultiplier: number;
    defenceLevelMultiplier: number;
    finalAttackRollMultiplier: number;
    finalDefenceRollMultiplier: number;
    finalDamageMultiplier: number;
    magicBaseMaximumHit: number;
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

export interface DpsSampleResult extends DpsResult {
    accuracyPassed: boolean;
    sampledDamage: number;
}

export interface DpsSampleAggregateResult {
    attackCount: number;
    totalSampledDamage: number;
    averageSampledDamagePerAttack: number;
    sampledDps: number;
}

export interface DpsService {
    CalculateExpected(request: DpsRequest): DpsResult;
    SetSeed(seed: number): void;
    SampleSingleAttack(request: DpsRequest): DpsSampleResult;
    SampleSingleAttackWithSeed(
        request: DpsRequest,
        seed: number,
    ): DpsSampleResult;
    SampleAttacks(
        request: DpsRequest,
        attackCount: number,
    ): DpsSampleAggregateResult;
    SampleAttacksWithSeed(
        request: DpsRequest,
        attackCount: number,
        seed: number,
    ): DpsSampleAggregateResult;
}

export interface DpsServiceConstructor {
    new (): DpsService;
    CalculateEffectiveLevel(
        level: number,
        prayerMultiplier: number,
        levelMultiplier: number,
        styleBonus: number,
    ): number;
    CalculateAttackRoll(
        effectiveAttackLevel: number,
        offensiveEquipmentBonus: number,
        finalAttackRollMultiplier: number,
    ): number;
    CalculateDefenceRoll(
        effectiveDefenceLevel: number,
        defensiveEquipmentBonus: number,
        finalDefenceRollMultiplier: number,
    ): number;
    CalculateStandardMaximumHit(
        effectiveStrengthLevel: number,
        strengthEquipmentBonus: number,
        finalDamageMultiplier: number,
    ): number;
    CalculateHitChance(attackRoll: number, defenceRoll: number): number;
}

export interface EquipmentDatabase {
    HasEquipmentPiece(id: number): boolean;
    GetEquipmentPiece(id: number): EquipmentPiece;
    GetAllEquipmentPieces(): EquipmentPieceVector;
    GetEquipmentPiecesBySlot(slot: EquipmentSlot): EquipmentPieceVector;
}

export interface EquipmentDatabaseConstructor {
    new (): EquipmentDatabase;
    LoadFromJson(json: string): EquipmentDatabase;
}

export interface WeaponRecord {
    weapon: WeaponDefinition;
    name: string;
    attackCallbackName: string;
}

export interface WeaponDatabase {
    HasWeaponRecord(id: number | bigint): boolean;
    GetWeaponRecord(id: number | bigint): WeaponRecord;
}

export interface DatabaseService {
    GetEquipmentDatabase(): EquipmentDatabase;
    GetWeaponDatabase(): WeaponDatabase;
    GetCombatCompositionDatabase(): CombatCompositionDatabase;
    GetNpcDatabase(): NpcDatabase;
}

export interface DatabaseServiceConstructor {
    new (): DatabaseService;
    LoadFromJsonDocuments(
        manifestJson: string,
        equipmentJson: string,
        weaponsJson: string,
        combatCompositionsJson: string,
        npcsJson: string,
    ): DatabaseService;
}

export interface CombatCompositionDatabase {
    HasCombatCompositionRecord(id: number | bigint): boolean;
    GetCombatCompositionRecord(id: number | bigint): CombatCompositionRecord;
    GetAllCombatCompositionRecords(): CombatCompositionRecordVector;
    GetCombatCompositionRecordsBySource(
        source: CombatCompositionSource,
    ): CombatCompositionRecordVector;
    LoadSavedCombatCompositionRecordsFromJson(
        json: string,
        weaponDatabase: WeaponDatabase,
    ): void;
    ExportSavedCombatCompositionRecordsToJson(): string;
    CreateSavedCombatCompositionRecord(
        name: string,
        composition: CombatComposition,
    ): bigint;
    UpdateSavedCombatCompositionRecord(
        id: number | bigint,
        name: string,
        composition: CombatComposition,
    ): void;
    DeleteSavedCombatCompositionRecord(id: number | bigint): boolean;
}

export interface NpcDatabase {
    HasNpcDefinition(id: number | bigint): boolean;
    GetNpcDefinition(id: number | bigint): NpcDefinition;
    GetAllNpcDefinitions(): NpcDefinitionVector;
}

export interface EquipmentSet {
    SetEquipmentPiece(piece: EquipmentPiece): void;
    HasEquipmentPiece(slot: EquipmentSlot): boolean;
    GetEquipmentPiece(slot: EquipmentSlot): EquipmentPiece;
    GetEquipmentPieces(): EquipmentPieceVector;
    GetEquipmentBonuses(): EquipmentBonuses;
    BuildCombatComposition(
        stats: CombatStats,
        attackType: AttackType,
        magicBaseMaximumHit: number,
        weaponDatabase: WeaponDatabase,
    ): CombatComposition;
    BuildAttackComposition(
        stats: CombatStats,
        attackType: AttackType,
        weaponDatabase: WeaponDatabase,
    ): AttackComposition;
    BuildDefenceComposition(stats: CombatStats): DefenceComposition;
}

export interface EquipmentSetConstructor {
    new (): EquipmentSet;
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
    CreatePlayer(
        size: number,
        speed: number,
        combatComposition: CombatComposition,
    ): number;
    CreateNpc(
        size: number,
        speed: number,
        combatComposition: CombatComposition,
    ): number;
    PlaceActor(actorId: number, sceneId: number, coordinate: SceneCoordinate): boolean;
    RemoveActor(actorId: number): boolean;
    SetActorCombatComposition(
        actorId: number,
        combatComposition: CombatComposition,
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
    GetProjectileSnapshotsJson(): string;
}

export type CardinalDirection = number | "North" | "East" | "South" | "West";
export type AttackType =
    | number
    | "Stab"
    | "Slash"
    | "Crush"
    | "Magic"
    | "RangedLight"
    | "RangedStandard"
    | "RangedHeavy";
export type DefenderKind = number | "Player" | "Npc";
export type EquipmentSlot =
    | number
    | "Head"
    | "Cape"
    | "Amulet"
    | "Weapon"
    | "Body"
    | "Shield"
    | "Legs"
    | "Hands"
    | "Feet"
    | "Ring"
    | "Ammo";
export type CombatCompositionSource = number | "BuiltIn" | "Saved";

export interface EngineModule {
    Engine: new () => Engine;
    DevelopmentPlayerChaseScenario?: new () => DevelopmentPlayerChaseScenario;
    DpsService: DpsServiceConstructor;
    EquipmentDatabase: EquipmentDatabaseConstructor;
    DatabaseService: DatabaseServiceConstructor;
    EquipmentSet: EquipmentSetConstructor;
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
        Magic: AttackType;
        RangedLight: AttackType;
        RangedStandard: AttackType;
        RangedHeavy: AttackType;
    };
    DefenderKind: {
        Player: DefenderKind;
        Npc: DefenderKind;
    };
    EquipmentSlot: {
        Head: EquipmentSlot;
        Cape: EquipmentSlot;
        Amulet: EquipmentSlot;
        Weapon: EquipmentSlot;
        Body: EquipmentSlot;
        Shield: EquipmentSlot;
        Legs: EquipmentSlot;
        Hands: EquipmentSlot;
        Feet: EquipmentSlot;
        Ring: EquipmentSlot;
        Ammo: EquipmentSlot;
    };
    CombatCompositionSource: {
        BuiltIn: CombatCompositionSource;
        Saved: CombatCompositionSource;
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
