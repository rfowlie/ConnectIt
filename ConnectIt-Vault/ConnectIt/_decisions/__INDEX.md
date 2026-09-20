# ConnectIt — decisions

One dated note per settled, non-obvious call, so it doesn't get silently re-argued.
Governed by [[_core/_schema/_decisions|_core/_schema/_decisions.md]]. Append-only; a
superseded note is marked, not deleted.

Newest first:

| Date | Decision | Status |
|---|---|---|
| [2026-09-20-effective-per-turn-cap-lives-in-runtime-state](2026-09-20-effective-per-turn-cap-lives-in-runtime-state.md) | An action's effective per-turn cap is runtime state (seeded once from config, not reset per turn), read by the gate and the UI; modifier machinery deferred | Active |
| [2026-09-20-legacy-action-system-removed-stage-3](2026-09-20-legacy-action-system-removed-stage-3.md) | Stage 3: legacy `Actions`/`bIsRequired`/per-action caps/`RequiredActionTagA/B`/SWAP budget deleted; the Mediator gate is now mandatory and seeding failures are loud | Active |
| [2026-09-20-grid-x-horizontal-y-vertical-line-function-fixed](2026-09-20-grid-x-horizontal-y-vertical-line-function-fixed.md) | Grid convention X = left-right, Y = up-down; `GetTilesByDirection` fixed to walk Y for Up/Down and X for Left/Right, fixing Board Shift's degenerate sort and direction inversion | Active |
| [2026-09-20-grid-x-is-row-y-is-column-direction-table](2026-09-20-grid-x-is-row-y-is-column-direction-table.md) | (Superseded same day) X = row, Y = column with the direction table rewritten — a misreading of the map layout | Superseded |
| [2026-09-18-server-gate-in-mediator-and-state-revision](2026-09-18-server-gate-in-mediator-and-state-revision.md) | Per-action gate/spend lives in the Mediator's `ProcessRequest`; PlayerState gains an `ActionStateRevision` for exact limbo ordering; `ProducesRequestType` validates the client-supplied `ActionTag` | Active |
| [2026-09-18-skin-plugin-ships-a-generic-debug-panel](2026-09-18-skin-plugin-ships-a-generic-debug-panel.md) | Skin plugin ships a generic code-built debug panel (`SkinMechanics.ToggleDebug`) plus `GetSkinResolution` / `GetConfigurationIssues` — catalog validation delivered | Active |
| [2026-09-18-action-state-single-authoritative-counter-with-limbo](2026-09-18-action-state-single-authoritative-counter-with-limbo.md) | Per-action use counts live only on the plugin `ATurnBasedPlayerState`; after a confirmed action completes the component enters limbo until `OnActionRuntimeStateUpdated`, then checks auto-end | Active |
| [2026-09-18-skin-plugin-defines-no-row-shapes-project-owns-them](2026-09-18-skin-plugin-defines-no-row-shapes-project-owns-them.md) | Skin plugin defines no skin/catalog types — one project-defined DataTable per category (row name = ID), one tiny preset base row, subsystem + save slot resolve selection; level-forced runtime-only | Active |
| [2026-09-18-skin-catalogs-as-datatable-arrays-and-plugin-name](2026-09-18-skin-catalogs-as-datatable-arrays-and-plugin-name.md) | Skin plugin is `UnrealSkinMechanics`; skins/presets are arrays of DataTables in `USkinSettings` (row name = stable ID); per-category payloads stay typed data assets | Active |
| [2026-09-18-action-config-keyed-by-class-not-tag](2026-09-18-action-config-keyed-by-class-not-tag.md) | Action config and turn-end leaves reference `TSubclassOf<UTurnBasedAction>`; the tag is derived from the class default object, not authored twice | Active |
| [2026-09-18-numbered-actions-are-grantable-and-runtime-state-split](2026-09-18-numbered-actions-are-grantable-and-runtime-state-split.md) | Numbered actions start at 0 uses and can be granted mid-match, capped by `MaxHeldUses`; runtime state split into permanent/numbered structs | Active |
| [2026-09-18-faction-visuals-are-a-curated-skin-group](2026-09-18-faction-visuals-are-a-curated-skin-group.md) | Faction visuals are not a bespoke subsystem — they're a curated skin group (a skin keyed by faction slot); skin groups are how adventure mode forces a look per level | Active |
| [2026-09-18-skin-system-new-plugin-and-explicit-settings-lists](2026-09-18-skin-system-new-plugin-and-explicit-settings-lists.md) | Skin system lives in a new dedicated plugin; available skins/presets are explicit `UDeveloperSettings` lists (Asset Manager scan deferred) | Active |
| [2026-09-18-skins-local-only-and-data-on-fixed-actor-class](2026-09-18-skins-local-only-and-data-on-fixed-actor-class.md) | Skin choices are local-only (not replicated); a skin is data applied to a fixed actor class, never a class swap, so pools stay valid | Active |
| [2026-09-18-skin-system-layered-data-and-flow](2026-09-18-skin-system-layered-data-and-flow.md) | Skins: tag categories → skin assets → presets → ID-based player selection → `UGameInstanceSubsystem` resolver (level-forced > override > preset > default); consumers pull + `OnSkinChanged` | Active |
| [2026-09-18-turn-end-tree-as-instanced-uobject-nodes](2026-09-18-turn-end-tree-as-instanced-uobject-nodes.md) | Turn-end requirement tree uses Instanced UObject nodes (Leaf/Group), not the recursive struct — UHT rejects structs containing arrays of themselves | Active |
| [2026-09-18-scoring-line-visuals-must-cover-all-mutated-positions](2026-09-18-scoring-line-visuals-must-cover-all-mutated-positions.md) | Any action that can complete a scoring line must feed the scoring-line visual update every position it mutated — a swap-completed line once hid the completing piece while board state kept it | Active |
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
