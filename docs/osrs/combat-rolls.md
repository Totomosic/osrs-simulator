# OSRS Combat Attack/Defence Roll Model

## Purpose

This document describes the Old School RuneScape combat roll model for use in a C++ simulation engine. It focuses on:

* Attack rolls
* Defence rolls
* Hit chance
* Maximum hit rolls
* How player stats, boosted stats, gear bonuses, prayers, set effects, special attacks, and target-specific modifiers should be applied

The model is intended for deterministic simulation of OSRS-style PvM and PvP combat. It should be implemented as a data-driven system because many OSRS items, monsters, spells, and special attacks have unique overrides.

---

## 1. Core Concepts

OSRS combat generally separates an attack into two stages:

1. **Accuracy stage**

   * Compute the attacker's `attackRoll`.
   * Compute the defender's `defenceRoll`.
   * Compare the two rolls to determine `hitChance`.

2. **Damage stage**

   * Compute the attacker's `maxHit`.
   * If the attack passes accuracy, roll damage from `0` to `maxHit`, inclusive.
   * If the attack fails accuracy, damage is `0`.

For expected-value simulation:

```text
expectedDamagePerAttack = hitChance * (maxHit / 2.0)
```

For DPS:

```text
secondsPerAttack = weaponAttackSpeedTicks * 0.6
dps = expectedDamagePerAttack / secondsPerAttack
```

---

## 2. Data Model

A simulation engine should separate **base stats**, **current boosted stats**, **equipment bonuses**, **combat style**, and **contextual modifiers**.

### 2.1 Entity Stats

```cpp
struct CombatStats {
    int attack;
    int strength;
    int defence;
    int ranged;
    int magic;
    int hitpoints;
};
```

Use current boosted or drained values for combat rolls, not unboosted base levels.

Example:

```text
Base Attack = 99
Current boosted Attack = 118
Use 118 in the attack roll.
```

---

### 2.2 Equipment Bonuses

OSRS equipment exposes offensive, defensive, and damage bonuses.

```cpp
struct EquipmentBonuses {
    // Offensive accuracy bonuses
    int stabAttack;
    int slashAttack;
    int crushAttack;
    int magicAttack;
    int rangedAttack;

    // Defensive bonuses
    int stabDefence;
    int slashDefence;
    int crushDefence;
    int magicDefence;
    int rangedDefenceLight;
    int rangedDefenceStandard;
    int rangedDefenceHeavy;

    // Damage bonuses
    int meleeStrength;
    int rangedStrength;

    // Usually represented as percent or basis points.
    // Example: 15.0 means +15% magic damage.
    double magicDamagePercent;
};
```

Older or simpler systems may model ranged defence as a single value. Modern OSRS NPC data may distinguish ranged defence types such as light, standard, and heavy. For high-fidelity PvM simulation, support separate ranged defence types.

---

### 2.3 Attack Type

```cpp
enum class AttackType {
    Stab,
    Slash,
    Crush,
    Magic,
    RangedLight,
    RangedStandard,
    RangedHeavy
};
```

The attack type determines which offensive bonus and defensive bonus are used.

Examples:

```text
Melee stab attack:
  attacker bonus = stabAttack
  defender bonus = stabDefence

Magic attack:
  attacker bonus = magicAttack
  defender bonus = magicDefence

Ranged heavy attack:
  attacker bonus = rangedAttack
  defender bonus = rangedDefenceHeavy
```

---

## 3. Effective Levels

Combat rolls use an **effective level**, not just the visible stat.

A generic effective level pipeline is:

```text
effectiveLevel = floor(currentLevel * prayerMultiplier * levelMultipliers)
effectiveLevel += combatStyleBonus
effectiveLevel += 8
```

Where:

* `currentLevel` is the current boosted or drained skill level.
* `prayerMultiplier` comes from active prayers.
* `levelMultipliers` includes some set effects or contextual effects that act as effective-level multipliers.
* `combatStyleBonus` comes from the selected attack style.
* `+8` is a fixed OSRS roll constant.

Use integer flooring at OSRS-compatible points.

---

## 4. Combat Style Bonuses

Combat styles add small effective-level bonuses.

Typical melee examples:

```text
Accurate:
  +3 Attack

Aggressive:
  +3 Strength

Controlled:
  +1 Attack
  +1 Strength
  +1 Defence

Defensive:
  +3 Defence
```

Typical ranged examples:

```text
Accurate:
  +3 Ranged

Rapid:
  +0

Longrange:
  +3 Defence
```

Magic usually receives:

```text
Magic attack:
  +0 Magic

Defensive casting:
  +3 Defence for defensive XP mode
```

The engine should represent style bonuses explicitly rather than hard-coding them into weapon logic.

```cpp
struct StyleBonus {
    int attack = 0;
    int strength = 0;
    int defence = 0;
    int ranged = 0;
    int magic = 0;
};
```

---

## 5. Attack Roll

### 5.1 Generic Attack Roll Formula

```text
attackRoll = effectiveAttackLevel * (offensiveEquipmentBonus + 64)
```

Then apply accuracy multipliers from special attacks, set effects, target-specific effects, and other conditional modifiers.

```text
finalAttackRoll = floor(attackRoll * accuracyMultiplier1 * accuracyMultiplier2 * ...)
```

For exact OSRS parity, each modifier should define whether it applies:

* To the effective level
* To the equipment bonus
* To the final roll
* Before or after another modifier
* With floor after application

A practical simulation engine should support all of these modifier stages.

---

### 5.2 Melee Attack Roll

For melee:

```text
currentLevel = current Attack level
prayerMultiplier = melee attack prayer multiplier
styleBonus = melee attack style bonus
offensiveEquipmentBonus = stab/slash/crush attack bonus
```

Formula:

```text
effectiveAttack =
    floor(currentAttack * attackPrayerMultiplier * effectiveLevelMultipliers)
    + attackStyleBonus
    + 8

baseAttackRoll =
    effectiveAttack * (selectedMeleeAttackBonus + 64)

finalAttackRoll =
    floor(baseAttackRoll * finalAccuracyMultipliers)
```

Example attack bonus selection:

```text
Attack type = Slash
Use equipment.slashAttack
Defender uses equipment.slashDefence or NPC slash defence.
```

---

### 5.3 Ranged Attack Roll

For ranged:

```text
currentLevel = current Ranged level
prayerMultiplier = ranged prayer multiplier
styleBonus = ranged style bonus
offensiveEquipmentBonus = ranged attack bonus
```

Formula:

```text
effectiveRangedAttack =
    floor(currentRanged * rangedPrayerMultiplier * effectiveLevelMultipliers)
    + rangedStyleBonus
    + 8

baseAttackRoll =
    effectiveRangedAttack * (rangedAttackBonus + 64)

finalAttackRoll =
    floor(baseAttackRoll * finalAccuracyMultipliers)
```

The attack's ranged subtype determines the defender's defensive bonus:

```text
RangedLight     -> defender.rangedDefenceLight
RangedStandard  -> defender.rangedDefenceStandard
RangedHeavy     -> defender.rangedDefenceHeavy
```

---

### 5.4 Magic Attack Roll

For magic accuracy:

```text
currentLevel = current Magic level
prayerMultiplier = magic attack prayer multiplier
styleBonus = usually 0
offensiveEquipmentBonus = magic attack bonus
```

Formula:

```text
effectiveMagicAttack =
    floor(currentMagic * magicPrayerMultiplier * effectiveLevelMultipliers)
    + magicStyleBonus
    + 8

baseAttackRoll =
    effectiveMagicAttack * (magicAttackBonus + 64)

finalAttackRoll =
    floor(baseAttackRoll * finalAccuracyMultipliers)
```

Magic damage is usually spell-specific or weapon-specific and should not be inferred solely from the magic attack roll.

---

## 6. Defence Roll

### 6.1 Generic Defence Roll Formula

```text
defenceRoll = effectiveDefenceLevel * (defensiveEquipmentBonus + 64)
```

For players, effective defence usually includes prayer and style modifiers.

For NPCs, most standard PvM calculations use the monster's relevant defensive level and defence bonus. NPCs often do not have player-style defensive stance bonuses unless explicitly modelled.

---

### 6.2 Melee and Ranged Defence

Against melee and ranged attacks, player defence is based primarily on Defence level.

```text
effectiveDefence =
    floor(currentDefence * defencePrayerMultiplier * defenceLevelMultipliers)
    + defenceStyleBonus
    + 8

defenceRoll =
    effectiveDefence * (selectedDefenceBonus + 64)
```

Examples:

```text
Incoming stab attack:
  selectedDefenceBonus = stabDefence

Incoming slash attack:
  selectedDefenceBonus = slashDefence

Incoming ranged standard attack:
  selectedDefenceBonus = rangedDefenceStandard
```

For NPC targets, a common PvM formula is:

```text
npcDefenceRoll =
    (npcDefenceLevel + 9) * (npcSelectedDefenceBonus + 64)
```

This is equivalent to using:

```text
effectiveNpcDefence = npcDefenceLevel + 9
```

instead of the player-style:

```text
floor(level * prayer) + styleBonus + 8
```

---

### 6.3 Magic Defence

Magic defence differs between players and NPCs.

#### Player defending against magic

A player's magic defence effective level is based on both Magic and Defence:

```text
effectiveMagicDefenceLevel =
    floor(currentMagic * magicDefenceComponentMultiplier)
    + floor(currentDefence * defencePrayerMultiplier * defenceComponentMultiplier)
    + defenceStyleBonus
    + 8
```

Common component weights:

```text
Magic component:   70%
Defence component: 30%
```

Simplified:

```text
effectiveMagicDefence =
    floor(currentMagic * 0.70)
    + floor(currentDefence * defencePrayerMultiplier * 0.30)
    + defenceStyleBonus
    + 8
```

Then:

```text
magicDefenceRoll =
    effectiveMagicDefence * (magicDefenceBonus + 64)
```

#### NPC defending against magic

NPC magic defence is generally based on the NPC's Magic level and magic defence bonus, not its Defence level.

Common PvM formula:

```text
npcMagicDefenceRoll =
    (npcMagicLevel + 9) * (npcMagicDefenceBonus + 64)
```

---

## 7. Hit Chance

Once `attackRoll` and `defenceRoll` are known, hit chance is calculated with a two-branch formula.

If:

```text
attackRoll > defenceRoll
```

Then:

```text
hitChance =
    1.0 - (defenceRoll + 2.0) / (2.0 * (attackRoll + 1.0))
```

Otherwise:

```text
hitChance =
    attackRoll / (2.0 * (defenceRoll + 1.0))
```

Clamp defensively:

```cpp
hitChance = std::clamp(hitChance, 0.0, 1.0);
```

---

## 8. Maximum Hit

The maximum hit uses the attacker's offensive damage stat and damage bonus.

Accuracy and max hit are separate:

```text
Attack level affects melee accuracy.
Strength level affects melee max hit.

Ranged level affects both ranged accuracy and ranged max hit.

Magic level affects magic accuracy.
Magic max hit is usually spell-specific or powered-staff-specific.
```

---

### 8.1 Melee Max Hit

Generic melee max hit:

```text
effectiveStrength =
    floor(currentStrength * strengthPrayerMultiplier * effectiveLevelMultipliers)
    + strengthStyleBonus
    + 8

baseMaxHit =
    floor(0.5 + effectiveStrength * (meleeStrengthBonus + 64) / 640.0)

finalMaxHit =
    floor(baseMaxHit * damageMultipliers)
```

Where:

* `currentStrength` is boosted or drained Strength.
* `strengthPrayerMultiplier` comes from prayers such as Burst of Strength, Superhuman Strength, Ultimate Strength, Chivalry, or Piety.
* `meleeStrengthBonus` comes from gear.
* `damageMultipliers` includes effects such as Slayer helm, Salve amulet, special attack damage modifiers, monster weaknesses, or set effects.

---

### 8.2 Ranged Max Hit

Generic ranged max hit:

```text
effectiveRangedStrength =
    floor(currentRanged * rangedStrengthPrayerMultiplier * effectiveLevelMultipliers)
    + rangedStyleBonus
    + 8

baseMaxHit =
    floor(0.5 + effectiveRangedStrength * (rangedStrengthBonus + 64) / 640.0)

finalMaxHit =
    floor(baseMaxHit * damageMultipliers)
```

Important:

* Ranged accuracy uses `rangedAttackBonus`.
* Ranged max hit uses `rangedStrengthBonus`.
* Ammunition often contributes ranged strength.
* Some ranged weapons have built-in strength or special formulas.
* Some weapons ignore ammo strength or require specific ammo handling.

---

### 8.3 Magic Max Hit

Magic max hit is less uniform than melee and ranged.

Many standard spells have a fixed base max hit:

```text
baseSpellMaxHit = spell.maxHit
```

Then apply magic damage modifiers:

```text
finalMaxHit =
    floor(baseSpellMaxHit * (1.0 + magicDamagePercent / 100.0) * damageMultipliers)
```

However, many powered staves and special magic weapons use item-specific formulas based on Magic level, target type, charges, or special effects.

For implementation, magic max hit should be data-driven:

```cpp
struct MagicDamageFormula {
    enum class Type {
        FixedSpellMax,
        PoweredStaffFormula,
        ItemSpecialFormula,
        CustomScript
    };

    Type type;
    int fixedBaseMaxHit;
};
```

Do not assume all magic attacks use the same `effectiveLevel * bonus` formula as melee and ranged max hit.

---

## 9. Damage Roll

If an attack passes the accuracy check, OSRS-style damage is usually rolled uniformly:

```text
damage = randomInteger(0, maxHit)
```

This means a successful accuracy roll can still produce a `0`.

For expected damage:

```text
averageSuccessfulDamage = maxHit / 2.0
expectedDamage = hitChance * averageSuccessfulDamage
```

---

## 10. Prayers

Prayers usually apply as multipliers to effective levels before the final `+8`.

Examples:

```text
Melee attack prayer:
  affects Attack effective level

Melee strength prayer:
  affects Strength effective level

Ranged prayer:
  affects Ranged accuracy and Ranged max hit

Magic prayer:
  affects Magic accuracy and sometimes Magic damage, depending on prayer

Defence prayer:
  affects Defence effective level
```

Represent prayers as stat-specific multipliers:

```cpp
struct PrayerModifiers {
    double attackMultiplier = 1.0;
    double strengthMultiplier = 1.0;
    double defenceMultiplier = 1.0;
    double rangedAttackMultiplier = 1.0;
    double rangedStrengthMultiplier = 1.0;
    double magicAttackMultiplier = 1.0;
    double magicDamageMultiplier = 1.0;
};
```

Example:

```text
Piety:
  attackMultiplier   = 1.20
  strengthMultiplier = 1.23
  defenceMultiplier  = 1.25
```

Exact values should be stored in a prayer database.

---

## 11. Set Effects and Conditional Gear Effects

Many OSRS effects modify accuracy, max hit, or both.

Examples include:

* Void Knight equipment
* Elite Void
* Slayer helmet / Black mask
* Salve amulet variants
* Dragon hunter weapons
* Demonbane weapons
* Wilderness weapons
* Barrows set effects
* Inquisitor's armour
* Dharok's set
* Crystal armour with Bow of faerdhinen
* Tumeken's shadow
* Twisted bow
* Scythe of vitur
* Fang-style reroll mechanics
* Keris partisan variants
* Monster-specific weaknesses

These should not be hard-coded into the generic formula. Instead, model them as modifiers with explicit application stages.

---

### 11.1 Modifier Stages

A flexible engine should support at least these stages:

```cpp
enum class ModifierStage {
    EffectiveAttackLevel,
    EffectiveStrengthLevel,
    EffectiveDefenceLevel,

    OffensiveEquipmentBonus,
    DefensiveEquipmentBonus,

    FinalAttackRoll,
    FinalDefenceRoll,

    BaseMaxHit,
    FinalMaxHit,

    HitChance,
    DamageRoll,

    MultiHit,
    PostDamage
};
```

Example modifier:

```cpp
struct CombatModifier {
    ModifierStage stage;
    double multiplier = 1.0;
    int flatBonus = 0;

    bool appliesToMelee = false;
    bool appliesToRanged = false;
    bool appliesToMagic = false;

    bool requiresSlayerTask = false;
    bool requiresUndeadTarget = false;
    bool requiresDragonTarget = false;
    bool requiresDemonTarget = false;

    int priority = 0;
    bool floorAfterApplication = true;
};
```

---

### 11.2 Stacking Rules

Stacking is one of the most important details.

Not all bonuses stack. Examples:

```text
Slayer helmet and Salve amulet generally do not stack for the same target.
Some item effects override others.
Some effects apply only to accuracy.
Some apply only to damage.
Some apply to both.
Some apply before max-hit flooring.
Some apply after max-hit flooring.
```

Use a data-driven stacking group system:

```cpp
enum class StackingGroup {
    None,
    SlayerTaskAccuracy,
    SlayerTaskDamage,
    SalveAccuracy,
    SalveDamage,
    VoidAccuracy,
    VoidDamage,
    DragonbaneAccuracy,
    DragonbaneDamage,
    DemonbaneAccuracy,
    DemonbaneDamage,
    SpecialAttackAccuracy,
    SpecialAttackDamage
};
```

Then define rules such as:

```cpp
struct StackingRule {
    StackingGroup group;
    enum class Resolution {
        StackMultiplicatively,
        UseHighestOnly,
        MutuallyExclusive,
        OverrideLowerPriority
    } resolution;
};
```

---

## 12. Special Attacks

Special attacks may affect:

* Accuracy roll
* Max hit
* Damage roll distribution
* Number of hits
* Defence reduction
* Minimum hit
* Target-specific scaling
* Guaranteed hit behaviour
* Delayed damage
* Area damage
* Healing or recoil

Do not represent specials as just a single damage multiplier.

---

### 12.1 Special Attack Data Model

```cpp
struct SpecialAttackDefinition {
    std::string name;

    double accuracyMultiplier = 1.0;
    double damageMultiplier = 1.0;

    bool modifiesMaxHit = false;
    bool modifiesDamageRoll = false;
    bool hasMinimumHit = false;
    int minimumHit = 0;

    int hitCount = 1;
    bool independentAccuracyRolls = false;
    bool independentDamageRolls = true;

    bool drainsTargetStats = false;
    bool usesCustomFormula = false;
};
```

---

### 12.2 Common Special Attack Patterns

#### Accuracy multiplier only

```text
finalAttackRoll = floor(baseAttackRoll * specialAccuracyMultiplier)
```

#### Damage multiplier only

```text
finalMaxHit = floor(baseMaxHit * specialDamageMultiplier)
```

#### Accuracy and damage multiplier

```text
finalAttackRoll = floor(baseAttackRoll * specialAccuracyMultiplier)
finalMaxHit = floor(baseMaxHit * specialDamageMultiplier)
```

#### Multiple hits

For a two-hit special:

```cpp
for each hit:
    compute hit chance
    roll accuracy
    if hit:
        roll damage
```

Some multi-hit attacks split damage from one roll; others roll independently. This must be item-specific.

#### Defence-reducing specials

Some specials reduce the target's Defence or combat stats after damage is applied.

The simulation should process this as a post-hit effect:

```text
1. Roll accuracy.
2. Roll damage.
3. If special effect condition is met, reduce target stat.
4. Future attacks use the reduced current stat.
```

---

## 13. Monster-Specific Mechanics

Many monsters do not use only generic rolls.

Examples of special cases:

* Damage caps
* Flat armour
* Defence reductions
* Elemental weaknesses
* Ranged defence split by projectile type
* Prayer protection effects
* Phase-specific stats
* Shielded or invulnerable phases
* Reduced damage from certain styles
* Increased damage from specific weapons
* Minimum hit rules
* Attack style immunities

The engine should support monster modifiers:

```cpp
struct MonsterCombatRules {
    bool hasDamageCap = false;
    int damageCap = 0;

    double meleeDamageTakenMultiplier = 1.0;
    double rangedDamageTakenMultiplier = 1.0;
    double magicDamageTakenMultiplier = 1.0;

    double stabAccuracyTakenMultiplier = 1.0;
    double slashAccuracyTakenMultiplier = 1.0;
    double crushAccuracyTakenMultiplier = 1.0;

    bool immuneToPoison = false;
    bool immuneToVenom = false;

    bool customDefenceFormula = false;
};
```

---

## 14. Protection Prayers and Damage Reduction

Protection prayers do not usually change the attacker's attack roll. They modify damage after a hit is determined.

Typical handling:

```text
1. Compute attack roll.
2. Compute defence roll.
3. Roll hit chance.
4. Roll damage.
5. Apply protection prayer reduction.
6. Apply other post-damage reductions.
```

Represent this separately from accuracy:

```cpp
struct DamageReduction {
    AttackType type;
    double multiplier;
    int flatReduction;
};
```

Example:

```text
If Protect from Melee reduces incoming PvM melee damage by 100%:
  finalDamage = floor(rolledDamage * 0.0)

If a boss partially penetrates prayer:
  finalDamage = floor(rolledDamage * prayerPenetrationMultiplier)
```

PvP protection prayers often reduce damage rather than fully blocking it, depending on context.

---

## 15. Recommended Calculation Pipeline

A robust C++ implementation should use the following order.

### 15.1 Accuracy Pipeline

```text
1. Determine attack style.
2. Determine attack type.
3. Select attacker accuracy stat:
   - Attack for melee
   - Ranged for ranged
   - Magic for magic

4. Select attacker's offensive equipment bonus.
5. Compute effective attack level:
   - current stat
   - prayer multiplier
   - effective-level modifiers
   - style bonus
   - +8

6. Compute base attack roll:
   effectiveAttackLevel * (offensiveBonus + 64)

7. Apply final attack-roll modifiers:
   - set effects
   - target-specific weapon effects
   - special attack accuracy modifiers

8. Select defender effective defence level.
9. Select defender equipment or NPC defence bonus.
10. Compute base defence roll.
11. Apply final defence-roll modifiers.
12. Compute hit chance.
```

---

### 15.2 Damage Pipeline

```text
1. Determine damage style:
   - melee
   - ranged
   - magic

2. Compute base max hit:
   - melee: Strength level and melee strength bonus
   - ranged: Ranged level and ranged strength bonus
   - magic: spell or weapon formula

3. Apply max-hit modifiers:
   - prayers
   - set effects
   - target-specific effects
   - special attack damage modifiers

4. Apply damage caps or minimum-hit rules.
5. Roll accuracy.
6. If missed:
   damage = 0

7. If hit:
   damage = random integer from 0 to maxHit unless custom distribution applies.

8. Apply post-roll damage modifiers:
   - protection prayers
   - monster damage reduction
   - absorption
   - phase mechanics

9. Apply post-hit effects:
   - stat drains
   - healing
   - recoil
   - poison
   - venom
   - burn
```

---

## 16. C++-Style Pseudocode

```cpp
double ComputeHitChance(int attackRoll, int defenceRoll) {
    if (attackRoll > defenceRoll) {
        return 1.0 - (static_cast<double>(defenceRoll) + 2.0)
                   / (2.0 * (static_cast<double>(attackRoll) + 1.0));
    }

    return static_cast<double>(attackRoll)
         / (2.0 * (static_cast<double>(defenceRoll) + 1.0));
}
```

---

```cpp
int ComputeAttackRoll(
    int currentLevel,
    double prayerMultiplier,
    double effectiveLevelMultiplier,
    int styleBonus,
    int equipmentAttackBonus,
    const std::vector<double>& finalRollMultipliers
) {
    int effectiveLevel =
        static_cast<int>(std::floor(currentLevel * prayerMultiplier * effectiveLevelMultiplier))
        + styleBonus
        + 8;

    int roll = effectiveLevel * (equipmentAttackBonus + 64);

    for (double multiplier : finalRollMultipliers) {
        roll = static_cast<int>(std::floor(roll * multiplier));
    }

    return roll;
}
```

---

```cpp
int ComputeStandardMaxHit(
    int currentDamageLevel,
    double prayerMultiplier,
    double effectiveLevelMultiplier,
    int styleBonus,
    int strengthBonus,
    const std::vector<double>& maxHitMultipliers
) {
    int effectiveLevel =
        static_cast<int>(std::floor(currentDamageLevel * prayerMultiplier * effectiveLevelMultiplier))
        + styleBonus
        + 8;

    int maxHit = static_cast<int>(
        std::floor(0.5 + (effectiveLevel * (strengthBonus + 64)) / 640.0)
    );

    for (double multiplier : maxHitMultipliers) {
        maxHit = static_cast<int>(std::floor(maxHit * multiplier));
    }

    return maxHit;
}
```

---

```cpp
int RollDamage(
    bool passedAccuracy,
    int maxHit,
    Random& rng
) {
    if (!passedAccuracy) {
        return 0;
    }

    return rng.UniformInt(0, maxHit);
}
```

---

## 17. Integer Flooring Rules

OSRS combat formulas are highly sensitive to flooring.

Implementation guidance:

```text
Use integer arithmetic where possible.
Floor after each known OSRS stage.
Do not combine multipliers algebraically unless verified.
Do not delay flooring until the end unless the specific formula does so.
```

Bad:

```cpp
maxHit = floor(base * prayer * salve * slayer * special);
```

Better:

```cpp
effective = floor(level * prayer);
effective = floor(effective * setBonus);
effective += styleBonus + 8;

maxHit = floor(0.5 + effective * (bonus + 64) / 640.0);
maxHit = floor(maxHit * allowedDamageModifier);
maxHit = floor(maxHit * specialModifier);
```

Exact floor points should be verified per item, spell, and special attack.

---

## 18. Implementation Strategy

Use a generic core formula plus data-driven overrides.

Recommended modules:

```text
CombatStats
EquipmentBonuses
CombatStyle
AttackType
PrayerDatabase
ItemEffectDatabase
MonsterDatabase
SpellDatabase
SpecialAttackDatabase
ModifierResolver
RollCalculator
DamageCalculator
SimulationEngine
```

Avoid hard-coding item names inside the roll calculator.

Instead:

```text
Input:
  attacker
  defender
  weapon
  spell
  attack style
  active prayers
  active set effects
  target attributes
  special attack state

Output:
  attack roll
  defence roll
  hit chance
  max hit
  expected damage
  sampled damage
  post-hit effects
```

---

## 19. Validation Checklist

When validating against an OSRS DPS calculator, compare:

```text
Effective attack level
Effective strength/ranged/magic level
Attack roll
Defence roll
Hit chance
Max hit
Expected damage per attack
DPS
Special attack expected hit
Multi-hit distributions
```

Test cases should include:

```text
Plain melee with no prayers
Melee with Piety
Ranged with Rigour
Magic with damage-boosting gear
Void set
Elite Void
Slayer helmet on task
Salve amulet against undead
Dragonbane weapon against dragons
Demonbane weapon against demons
Special attack with accuracy multiplier
Special attack with damage multiplier
Multi-hit weapon
Defence-draining special
Magic defence against player
Magic defence against NPC
Monster with damage cap
Monster with ranged defence subtype
```

---

## 20. Key Rules Summary

```text
Attack roll determines whether an attack lands.
Max hit determines the damage range if the attack lands.
Defence usually reduces accuracy, not max damage.
Melee accuracy uses Attack.
Melee max hit uses Strength.
Ranged accuracy and max hit both use Ranged, but with different gear bonuses.
Magic accuracy uses Magic.
Magic max hit is usually spell-specific or weapon-specific.
Gear accuracy bonuses affect attack rolls.
Gear strength bonuses affect max hits.
Defensive gear bonuses affect defence rolls.
Prayers usually multiply effective levels before style bonus and +8.
Special attacks may modify accuracy, damage, hit count, target stats, or damage distribution.
Many item effects are conditional and require data-driven stacking rules.
Flooring order matters.
```
