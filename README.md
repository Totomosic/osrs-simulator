# OSRS Simulator

Monorepo scaffold for deterministic Old School RuneScape simulation, repeated execution, optimization, and future visualization tooling.

## Project Layout

- `cpp/common`: shared C++ primitives.
- `web`: future TypeScript result viewer.
- `tools`: support scripts and developer tooling.
- `docs`: architecture notes and ADRs.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

## Boundary Rules

- `cpp/common` should stay limited to stable shared primitives.
- Future engine code should own deterministic simulation correctness.
- Future simulator code should depend on the engine and own repetition, aggregation, and statistics.
- Future apps should be thin adapters over libraries.
- Frontend code should remain separate from C++ implementation details.
