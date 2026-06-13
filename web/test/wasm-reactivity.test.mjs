import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { isProxy, ref, shallowRef } from "vue";
import createGeneratedEngineModule from "../src/wasm/generated/EngineModule.js";

const root = dirname(dirname(fileURLToPath(import.meta.url)));
const wasmBinary = await readFile(
    resolve(root, "src/wasm/generated/EngineModule.wasm"),
);
const module = await createGeneratedEngineModule({ wasmBinary });

{
    const rawScenario = new module.DevelopmentPlayerChaseScenario();
    const scenario = ref(rawScenario);

    assert.equal(isProxy(scenario.value), true);
    assert.throws(
        () => scenario.value.GetTick(),
        /Expected null or instance of DevelopmentPlayerChaseScenario, got an instance of DevelopmentPlayerChaseScenario/,
    );
}

{
    const rawScenario = new module.DevelopmentPlayerChaseScenario();
    const scenario = shallowRef(rawScenario);

    assert.equal(isProxy(scenario.value), false);
    assert.equal(scenario.value.GetTick(), 0n);
}
