import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = dirname(dirname(fileURLToPath(import.meta.url)));
const viewSource = await readFile(
    resolve(root, "src/DpsCalculatorView.vue"),
    "utf8",
);

const npcFormStart = viewSource.indexOf('aria-label="NPC defence setup"');
const resultsTableStart = viewSource.indexOf('class="dps-table"');

assert.notEqual(npcFormStart, -1);
assert.notEqual(resultsTableStart, -1);

const npcFormSource = viewSource.slice(npcFormStart, resultsTableStart);

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
