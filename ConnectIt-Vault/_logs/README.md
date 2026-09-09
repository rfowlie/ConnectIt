# _logs/ — index

Newest first. See [`_schema/logs.md`](../_schema/logs.md) for what governs this folder.

- [[2026-09-09-5]] — Debug cursor hover regression: root cause was `CI_LevelConfig_Test`'s `TileRegistry` slot never migrated off the plugin base class.
- [[2026-09-09-4]] — Added `UConnectIt_TileRegistry`/`UConnectIt_PieceRegistry` so registries can resolve board state on demand.
- [[2026-09-09-3]] — Explained `UDeveloperSettings` vs. `GameInstance` for level-config resolution (no changes).
- [[2026-09-09-2]] — Moved the blueprint function libraries to `Framework/Library/`.
- [[2026-09-09]] — Moved `TileRegistry`/`PieceRegistry` off the PlayerController onto a new `UConnectIt_BoardRegistrySubsystem`.
- [[2026-09-05]] — Board architecture overhaul: retired `AConnectIt_BoardManager`, board state → GameState, request/rules → GameMode `UObject`s, new level-config DataAsset, shift pipeline removed.
