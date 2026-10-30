# CLAUDE.md — UnrealPathMechanics

Path tracing: a tracer that lays path marks along a route, with its path-mark effects, UI and materials. Imported from the Path
Tracer demo as a **content-only** plugin. The plan is to convert its Blueprints to C++ and find each part a new home in our
framework. The goals and findings are in [PathTracer](../UnrealEngineTools/PathTracer.md).

## Domain

- **Source:** `../Plugins/UnrealPathMechanics/` — content only (about 250 assets: Blueprints, materials, meshes, showcase and demo
  content). No C++ source yet.
- **Status:** content only. Goals written 2026-10-30; nothing converted.
- **Type:** intended to be game-agnostic. Currently Blueprint only.
- **Consumed by:** nothing. It is not enabled in `ConnectIt.uproject`.

## Sections

None yet. Create `code/` (per [`_core/_schema/_code.md`](../_core/_schema/_code.md)) once there are C++ types.

## Next steps

- Phase 1: convert the core tracer to C++ and place it (see the goals in [PathTracer](../UnrealEngineTools/PathTracer.md)).
- Enable the plugin in `ConnectIt.uproject` only once it has code the game uses.
