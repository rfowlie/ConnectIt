# ConnectIt — decisions

One dated note per settled, non-obvious call, so it doesn't get silently re-argued.
Governed by [[_core/_schema/_decisions|_core/_schema/_decisions.md]]. Append-only; a
superseded note is marked, not deleted.

Newest first:

| Date | Decision | Status |
|---|---|---|
| [[ConnectIt/_decisions/2026-09-18-piece-registry-mapping-fix-re-keys-not-rebuilds\|2026-09-18-piece-registry-mapping-fix-re-keys-not-rebuilds]] | `PieceMap` re-keys from `FConnectItBoardChangeEvent`'s own positions, not actor transforms — the obvious `UpdateMappings()` rebuild would read stale pre-lerp data | Active |
| [[ConnectIt/_decisions/2026-09-18-generic-game-event-any-tag-complete-delegate\|2026-09-18-generic-game-event-any-tag-complete-delegate]] | `GameEventTaskSubsystem` gains `OnAnyTagComplete` — one generic completion delegate instead of a `BindOnTagComplete` call per tag | Active |
| [[ConnectIt/_decisions/2026-09-18-grid-definition-extracted-from-tile-registry\|2026-09-18-grid-definition-extracted-from-tile-registry]] | Grid geometry extracted off `UGridTileRegistryBase` into its own `UGridDefinition`, shared by both registries as siblings | Active |
| [[ConnectIt/_decisions/2026-09-18-visual-reactions-in-blueprint-convention\|2026-09-18-visual-reactions-in-blueprint-convention]] | Visual-event reactions in Blueprint, never C++, elevated from the board-shift case to a standing project convention | Active |
| [[ConnectIt/_decisions/2026-09-18-generic-turn-end-requirement-system\|2026-09-18-generic-turn-end-requirement-system]] | Recursive `FTurnEndRequirementNode` tree + split permanent/numbered action config replaces `RequiredActionTagA`/`B` and the base plugin's AND-everything model | Active |
| [[ConnectIt/_decisions/2026-09-18-loadout-reference-on-playerstate\|2026-09-18-loadout-reference-on-playerstate]] | `PlayerState` holds one shared `Loadout` reference (no per-player duplication); `ActionsComponent` stays the sole action-instance builder | Active |
| [[ConnectIt/_decisions/2026-09-18-registry-mapping-refresh-on-game-event-complete\|2026-09-18-registry-mapping-refresh-on-game-event-complete]] | `TileRegistry`/`PieceRegistry` subscribe to `GameEvent` completion and recompute position→actor mappings, fixing a confirmed post-shift desync bug | Active |
| [[ConnectIt/_decisions/2026-09-17-visual-events-live-in-blueprint-not-cpp\|2026-09-17-visual-events-live-in-blueprint-not-cpp]] | Visual reactions to gameplay events (tile/piece animation, VFX) live in Blueprint, not C++ — C++ exposes data + reusable tools only | Active |
| [[ConnectIt/_decisions/2026-09-14-level-config-default-fallback\|2026-09-14-level-config-default-fallback]] | `GetLevelConfig` falls back to a `DefaultLevelConfig` + warns, instead of erroring to null, when a level isn't registered | Active |
| [[ConnectIt/_decisions/2026-09-09-faction-visuals-subsystem-built-then-removed\|2026-09-09-faction-visuals-subsystem-built-then-removed]] | Build the faction-visuals subsystem, then remove it — stay white-box | Active |
| [[ConnectIt/_decisions/2026-09-09-board-registries-to-world-subsystem\|2026-09-09-board-registries-to-world-subsystem]] | Tile/piece registries live on a `UWorldSubsystem`, duplicated per world from level-config templates | Active |
| [[ConnectIt/_decisions/2026-09-08-board-architecture-overhaul\|2026-09-08-board-architecture-overhaul]] | Retire `AConnectIt_BoardManager`; split its jobs across GameMode / GameState / config | Active |
| [[ConnectIt/_decisions/2026-09-08-retire-legacy-mvvm-pipeline\|2026-09-08-retire-legacy-mvvm-pipeline]] | Keep the non-networked state-machine / facade / view-model pipeline as dead code, don't build on it | Active |
| [[ConnectIt/_decisions/2026-09-06-suite-conventions\|2026-09-06-suite-conventions]] | Cross-cutting conventions for the plugin suite (tags, delegates, RPCs, extension hook) | Active |

## Related

- [[ConnectIt/code/__INDEX|code/__INDEX.md]] — inventory, suite map, known issues
- [[ConnectIt/_logs/__INDEX|_logs/]] — ConnectIt-scoped maintenance passes
