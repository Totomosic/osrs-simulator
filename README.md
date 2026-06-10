# OSRS Simulator

Monorepo scaffold for deterministic Old School RuneScape simulation, repeated execution, optimization, and future visualization tooling.

## Project Layout

- `cpp/common`: shared C++ primitives.
- `web`: future TypeScript result viewer.
- `tools`: support scripts and developer tooling.
- `docs`: architecture notes and ADRs.

## Build

```sh
tools/build.sh
```

## C++ Logging

`cpp/common/src/Logging.h` provides the shared logger and convenience macros:

```cpp
#include "Logging.h"

LOG_INFO("Loaded {} entities", entityCount);
LOG_ERROR("Failed to load {}", path);
LOG_ASSERT(entityCount > 0, "Expected at least one entity");
```

## Boundary Rules

- `cpp/common` should stay limited to stable shared primitives.
- Future engine code should own deterministic simulation correctness.
- Future simulator code should depend on the engine and own repetition, aggregation, and statistics.
- Future apps should be thin adapters over libraries.
- Frontend code should remain separate from C++ implementation details.
