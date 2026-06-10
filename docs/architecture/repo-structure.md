# Repo Structure

This repository is organized as a monorepo scaffold for isolated packages that share the OSRS simulation domain.

## C++ Libraries

`cpp/common` contains the current C++ code: low-level shared types that are stable enough to be used across future packages.

Future `cpp/engine` code should contain deterministic world simulation: entities, maps, movement, player actions, NPC behavior, combat rules, tick advancement, and RNG boundaries.

Future `cpp/sim` code should contain higher-level simulation workflows: repeated runs, optimization, aggregation, and statistics.

## Apps

Future apps should stay thin and avoid owning simulation logic.

## Frontend

`web` is reserved for a TypeScript viewer. It should remain separate from C++ implementation details.
