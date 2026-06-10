# Repo Structure

This repository is organized as a monorepo with isolated packages that share the OSRS simulation domain.

## C++ Libraries

`cpp/common` contains low-level shared types that are stable enough to be used across packages.

`cpp/engine` contains deterministic world simulation: entities, maps, movement, player actions, NPC behavior, combat rules, tick advancement, and RNG boundaries.

`cpp/sim` contains higher-level simulation workflows: repeated runs, optimization, aggregation, and statistics.

## Apps

`apps/osrs-sim-cli` is the first executable wrapper around `cpp/sim`. It should stay thin and avoid owning simulation logic.

## Frontend

`web` is reserved for a TypeScript viewer. It should remain separate from C++ implementation details.
