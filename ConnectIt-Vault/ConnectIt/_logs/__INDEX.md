# ConnectIt/_logs/

ConnectIt-domain maintenance passes — automated changes scoped to the game module. One
timestamped note per run. Governed by [[_core/_schema/_logs|_core/_schema/_logs.md]].
Vault-wide passes go in `_core/_logs/`.

Newest first:

- [2026-09-14-0207](2026-09-14-0207.md) — session — applied the 2026-09-14 check-in's task
  candidates to `_tasks/`: 2 rows completed, 1 rewritten (root-action task → HUD-owned-UI
  confirmation), 4 added (Board Shift, turn-end AND/OR groups, blocker prototype,
  `DefaultLevelConfig` confirmation) — by explicit owner request, not the default
  candidates-only flow
- [2026-09-14-0044](2026-09-14-0044.md) — session — opened `ConnectIt/_questions/`,
  logged the first open question (`action-gameplay-tags` — 4-5 disconnected places an
  action tag has to be set), linked to the related `_tasks/active.md` fix
- [2026-09-14-0039](2026-09-14-0039.md) — session — `GetLevelConfig` now falls back to a
  new `DefaultLevelConfig` (+ warns) instead of erroring to null on an unregistered
  level; root-caused the missing-actions bug on the duplicated map; decision note added
- [2026-09-14-0018](2026-09-14-0018.md) — process-discussion — `swap-implementation-qa`
  filed from a compiled (not pasted) transcript — 5 SWAP debugging Q&A exchanges, 4 topics
  (1 continuing `action-state-architecture`, 3 first mentions)
- [2026-09-13-2342](2026-09-13-2342.md) — session — added `_discussions/__INDEX.md`
  (3 notes indexed), closing the flag from 2026-09-12-1943; linked it from
  `ConnectIt/CLAUDE.md`
- [2026-09-12-1943](2026-09-12-1943.md) — process-discussion — `action-state-architecture`
  filed from a pasted transcript, 4 topics (1 continuing the board-mediator discussion,
  3 first mentions); Questions-and-an-Update only, no Tasks/Deliverables/Workflows
- [2026-09-12-0102](2026-09-12-0102.md) — session — opened `ConnectIt/_tasks/` (README +
  active/complete/suspended), added it to `ConnectIt/CLAUDE.md`'s section list, seeded
  1 open task from the board-mediator discussion
- [2026-09-12-0044](2026-09-12-0044.md) — process-discussion — `board-mediator-and-action-config-design`
  filed from a pasted transcript, 7 topics (2 continuing the swap-action-ui-design note,
  5 first mentions)
- [2026-09-11-2346](2026-09-11-2346.md) — process-discussion — first `_discussions/` note:
  `swap-action-ui-design`, filed from a pasted transcript, 5 topics (all first mentions)
- [[2026-09-09-5]] — session — debugged cursor-hover regression: `CI_LevelConfig_Test.uasset` `TileRegistry` slot held the wrong (plugin base) type → nulled on load → no tiles registered with `UGridHoverSubsystem`. Diagnosis only; fixed manually.
- [[2026-09-09-4]] — session — `UConnectIt_TileRegistry` / `UConnectIt_PieceRegistry` subclasses added (each with on-demand `GetBoardState()`); level-config templates retyped.
- [[2026-09-09-3]] — session — explained `UConnectIt_LevelConfigSettings` (`UDeveloperSettings`) vs `GameInstance` for level→asset config resolution. Informational.
- [[2026-09-09-2]] — session — moved the 5 Blueprint function libraries `Library/` → `Framework/Library/`; fixed 17 dependents' includes.
- [[2026-09-09]] — session — `UConnectIt_BoardRegistrySubsystem` (`UWorldSubsystem`): one canonical registry pair per world, `DuplicateObject`'d from `UConnectIt_LevelConfigDataAsset` `Instanced` templates at `OnWorldBeginPlay`; registries removed from `AConnectIt_PlayerController`; `UGridPieceRegistryBase::ShutdownRegistry()` gap closed.
- [[2026-09-05]] — session — board-architecture overhaul: retired `AConnectIt_BoardManager`; board state → GameState; request/rules → GameMode-owned `UObject`s; `UConnectIt_LevelConfigDataAsset`/`Settings` added; dead shift pipeline deleted.
- [[2026-09-04-2]] — session — Blueprint-only guidance on a `CIW_ScoreBoard`/`CIW_ScoreBoard_Element` architecture (frame builds rows once, rows self-bind and resolve their own data); no code changed, plan never formally approved.
- [[2026-09-04]] — session — UI-value catalogue (`UIValueCatalogue.md`, now folded into the vault) + 7 accessor gaps closed + 2 correctness bugs fixed across `AConnectIt_GameState`/`UnrealTurnBasedMechanics`; faction-visuals subsystem (Gap 1) built then removed same session, see [[ConnectIt/_decisions/2026-09-09-faction-visuals-subsystem-built-then-removed|decision note]]. Predates the 2026-09-08 board-architecture overhaul — see log for what has since moved/retired.
