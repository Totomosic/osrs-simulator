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
    const rawEngine = new module.Engine();
    const engine = ref(rawEngine);

    assert.equal(isProxy(engine.value), true);
    assert.throws(
        () => engine.value.GetCurrentTick(),
        /Expected null or instance of Engine, got an instance of Engine/,
    );
}

{
    const rawEngine = new module.Engine();
    const engine = shallowRef(rawEngine);

    assert.equal(isProxy(engine.value), false);
    assert.equal(engine.value.GetCurrentTick(), 0n);
}
