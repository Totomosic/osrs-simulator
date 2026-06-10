# OSRS Simulator

Monorepo for deterministic Old School RuneScape simulation, repeated execution, optimization, and future visualization tooling.

## Project Layout

- `cpp/common`: shared C++ primitives used by engine and simulator packages.
- `cpp/engine`: deterministic OSRS world/action/rules engine.
- `cpp/sim`: repeated execution, optimization, aggregation, and statistics.
- `apps/osrs-sim-cli`: command-line simulator entry point.
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

- `cpp/engine` owns deterministic simulation correctness.
- `cpp/sim` depends on `cpp/engine` and owns repetition, aggregation, and statistics.
- Apps are thin adapters over libraries.
- Frontend code should remain separate from C++ implementation details.
