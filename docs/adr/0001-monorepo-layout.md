# 0001: Monorepo Layout

## Status

Accepted

## Context

The project needs a deterministic C++ engine, C++ simulation tools, future optimization workflows, and a TypeScript viewer.

## Decision

Use a monorepo with isolated top-level areas for C++ libraries, apps, tools, docs, and web code.

## Consequences

Shared contracts can evolve in one repository while package boundaries keep engine, simulator, frontend, and tooling concerns separate.
