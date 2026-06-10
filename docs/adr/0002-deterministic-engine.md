# 0002: Deterministic Engine

## Status

Accepted

## Context

Simulation behavior must be reproducible for debugging, regression testing, optimization, and visualization.

## Decision

The core engine must be deterministic for a given initial state, input stream, and RNG seed. Non-deterministic behavior belongs outside the engine boundary or must be injected explicitly.

## Consequences

Tests can assert exact behavior over ticks. Runs can be reproduced from explicit inputs and seeds.
