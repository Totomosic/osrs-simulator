# Web

Development viewer for inspecting simulator behavior in a browser.

## Workflow

Install JavaScript dependencies from the repository root:

```sh
yarn install
```

Build and type-check the Vue/TypeScript app:

```sh
yarn web:build
```

Build the C++ code with an Emscripten toolchain:

```sh
yarn wasm:build
```

`yarn wasm:build` expects `emcmake`, `em++`, and the rest of the Emscripten SDK to be active in the shell. It configures a separate `build/wasm` tree so native CMake builds stay separate from browser builds.

Generated WebAssembly outputs and Emscripten glue should be written under ignored artifact paths such as `web/public/wasm/` or `web/src/wasm/generated/`, not committed as source.

The development viewer may temporarily expose C++ engine details while the browser integration evolves. User-facing visualization should remain separate from C++ implementation details.
