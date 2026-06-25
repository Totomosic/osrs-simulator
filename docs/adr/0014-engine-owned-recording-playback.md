# Engine-Owned Recording Playback

## Status

Accepted

Recording playback should apply recorded encounter facts through engine world and actor APIs rather than reimplementing playback semantics in the web app. The web recording viewer will load recordings through engine WebAssembly and render recording-specific snapshots.

## Considered Options

- Parse and play recordings entirely in TypeScript.
- Re-run the encounter simulation from a recording.
- Add a C++ Recording Playback object that applies recorded facts without stepping the engine.

## Decision

Recording Playback lives in the C++ engine and is exposed through WebAssembly. It loads versioned recording JSON, validates schema and tick ordering, applies state through low-level APIs without calling `Engine::Step`, and supports random access by rebuilding from initial state and applying tick entries up to the requested tick.

## Consequences

Playback remains consistent between native tests and the web viewer, and invalid recordings fail loudly when facts cannot be applied. Playback is intentionally not a resumed active encounter and does not reconstruct encounter hooks, AI state, RNG state, or combat queue internals.
