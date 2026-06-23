# Encounter Runner Orchestrates Active Encounters

## Status

Accepted

An encounter is encounter-specific gameplay orchestration layered over one or more scenes, while `Engine` remains the generic tick executor. `EncounterRunner` owns one `Engine` and one `ActiveEncounter`, starts the encounter explicitly, and advances ticks by running encounter hooks around `Engine::Step()`.

## Considered Options

- Put encounter-wide logic inside `Engine::Step()`.
- Let each active encounter own and step an engine directly.
- Use a runner that owns both the engine and active encounter lifecycle.

## Decision

`EncounterRunner` owns its `Engine` and a uniquely owned `ActiveEncounter`. `Start()` calls mandatory `ActiveEncounter::Initialize(EncounterContext&)`; `Step()` checks completion before advancing, runs `BeforeEngineTick`, calls `Engine::Step()`, then runs `AfterEngineTick`. Only `EncounterRunner` advances ticks, and one active encounter runs on one engine instance.

NPC behavior remains actor-local and is updated inside the engine's NPC processing loop. Active encounters remain encounter-wide orchestration for scene preparation, player/NPC setup, phase state, timers, completion checks, and behavior assignment. `EncounterDefinition` is deferred until registry, UI selection, saved config, or batch creation needs it.

## Consequences

Encounter hooks can mutate simulation state before or after a tick without making `Engine` depend on boss-specific orchestration. Resetting an encounter in the first slice is done by destroying and recreating the runner rather than adding core reset semantics. Debug scenarios such as Player Chase may use encounter infrastructure without becoming domain encounters.
