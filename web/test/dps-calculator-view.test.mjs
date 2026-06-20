import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = dirname(dirname(fileURLToPath(import.meta.url)));
const viewSource = await readFile(
    resolve(root, "src/DpsCalculatorView.vue"),
    "utf8",
);
const calculatorSource = await readFile(
    resolve(root, "src/dpsCalculator.ts"),
    "utf8",
);

const npcFormStart = viewSource.indexOf('aria-label="NPC defence setup"');
const resultsTableStart = viewSource.indexOf('class="dps-table"');
const playerFormStart = viewSource.indexOf('aria-label="Player attack setup"');

assert.notEqual(npcFormStart, -1);
assert.notEqual(resultsTableStart, -1);
assert.notEqual(playerFormStart, -1);

const playerFormSource = viewSource.slice(playerFormStart, npcFormStart);
const npcFormSource = viewSource.slice(npcFormStart, resultsTableStart);

assert.match(playerFormSource, /v-model="activePlayerAttackSetup\.mode"/);
assert.match(playerFormSource, /value="manual"/);
assert.match(playerFormSource, /value="equipment"/);
assert.match(
    playerFormSource,
    /v-model\.number="activePlayerAttackSetup\.equipmentPieceIds\[slot\.key\]"/,
);
assert.match(playerFormSource, /equipmentModeSlotOptions/);
assert.match(viewSource, /getEquipmentModeSlotOptions/);
assert.match(viewSource, /loadEquipmentDataset/);
assert.match(viewSource, /fetchTextAsset\("manifest\.json"\)/);
assert.match(viewSource, /manifest\.documents\?\.equipment/);
assert.match(viewSource, /manifest\.documents\?\.combatCompositions/);
assert.match(viewSource, /manifest\.documents\?\.npcs/);
assert.match(viewSource, /equipmentSlotControls/);
assert.match(playerFormSource, /v-for="slot in equipmentModeSlotOptions"/);
for (const label of [
    "Head",
    "Cape",
    "Amulet",
    "Weapon",
    "Body",
    "Shield",
    "Legs",
    "Hands",
    "Feet",
    "Ring",
    "Ammo",
]) {
    assert.match(calculatorSource, new RegExp(`label: "${label}"`));
}
assert.match(playerFormSource, /v-if="activePlayerAttackSetup\.mode === 'manual'"/);
assert.match(
    playerFormSource,
    /v-if="activePlayerAttackSetup\.mode === 'equipment'"/,
);

assert.match(
    npcFormSource,
    /v-model\.number="calculatorState\.npcDefenceSetup\.defence"/,
);
assert.match(
    npcFormSource,
    /v-model\.number="calculatorState\.npcDefenceSetup\.magic"/,
);
assert.match(
    npcFormSource,
    /v-model\.number="calculatorState\.npcDefenceSetup\.stabDefence"/,
);
assert.match(
    npcFormSource,
    /v-model\.number="calculatorState\.npcDefenceSetup\.slashDefence"/,
);
assert.match(
    npcFormSource,
    /v-model\.number="calculatorState\.npcDefenceSetup\.crushDefence"/,
);
assert.match(
    npcFormSource,
    /v-model\.number="calculatorState\.npcDefenceSetup\.magicDefence"/,
);
assert.match(
    npcFormSource,
    /v-model\.number="calculatorState\.npcDefenceSetup\.rangedDefenceLight"/,
);
assert.match(
    npcFormSource,
    /v-model\.number="calculatorState\.npcDefenceSetup\.rangedDefenceStandard"/,
);
assert.match(
    npcFormSource,
    /v-model\.number="calculatorState\.npcDefenceSetup\.rangedDefenceHeavy"/,
);
assert.doesNotMatch(npcFormSource, /equipment/i);
assert.doesNotMatch(npcFormSource, /<select\b/);

const resultsTableSource = viewSource.slice(resultsTableStart);

assert.match(resultsTableSource, /v-for="\(\s*row,\s*rowIndex\s*\) in resultRows"/);
assert.match(resultsTableSource, /{{ row\.defenceRoll }}/);
assert.match(resultsTableSource, /{{ row\.dpsDifference }}/);
