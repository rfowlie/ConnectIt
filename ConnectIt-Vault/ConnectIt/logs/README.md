# ConnectIt/logs/

ConnectIt-domain maintenance passes — automated changes scoped to the game module. One
timestamped note per run. Governed by [[_core/_schema/_logs|_core/_schema/_logs.md]].
Vault-wide passes go in `_core/_logs/`.

Newest first:

- [[2026-09-09-5]] — session — debugged cursor-hover regression: `CI_LevelConfig_Test.uasset` `TileRegistry` slot held the wrong (plugin base) type → nulled on load → no tiles registered with `UGridHoverSubsystem`. Diagnosis only; fixed manually.
- [[2026-09-09-4]] — session — `UConnectIt_TileRegistry` / `UConnectIt_PieceRegistry` subclasses added (each with on-demand `GetBoardState()`); level-config templates retyped.
- [[2026-09-09-3]] — session — explained `UConnectIt_LevelConfigSettings` (`UDeveloperSettings`) vs `GameInstance` for level→asset config resolution. Informational.
- [[2026-09-09-2]] — session — moved the 5 Blueprint function libraries `Library/` → `Framework/Library/`; fixed 17 dependents' includes.
- [[2026-09-09]] — session — `UConnectIt_BoardRegistrySubsystem` (`UWorldSubsystem`): one canonical registry pair per world, `DuplicateObject`'d from `UConnectIt_LevelConfigDataAsset` `Instanced` templates at `OnWorldBeginPlay`; registries removed from `AConnectIt_PlayerController`; `UGridPieceRegistryBase::ShutdownRegistry()` gap closed.
- [[2026-09-05]] — session — board-architecture overhaul: retired `AConnectIt_BoardManager`; board state → GameState; request/rules → GameMode-owned `UObject`s; `UConnectIt_LevelConfigDataAsset`/`Settings` added; dead shift pipeline deleted.
