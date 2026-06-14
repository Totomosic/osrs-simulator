# Web Development Viewer

## Status

Accepted

The `web` package is currently a development viewer for inspecting and verifying engine behavior in a browser. It may expose C++ engine internals directly through WebAssembly while the engine model is still evolving; this is a deliberate exception to the long-term frontend boundary, where user-facing visualization should remain separate from C++ implementation details.

Development scenarios belong in the `web` package rather than as scenario-specific C++ engine classes. A web scenario should construct initial scene entities and actors through generic engine, world, and scene APIs exposed through WebAssembly, then advance simulation by calling `Engine::Step()` on the viewer's playback interval. Scenario names, viewer state, labels, blocked-click feedback, and snapshot assembly remain web concerns.
