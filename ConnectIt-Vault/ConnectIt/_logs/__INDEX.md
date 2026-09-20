# ConnectIt/_logs/

ConnectIt-domain maintenance passes — automated changes scoped to the game module. One
timestamped note per run. Governed by [[_core/_schema/_logs|_core/_schema/_logs.md]].
Vault-wide passes go in `_core/_logs/`.

Newest first:

- [2026-09-20-1449](2026-09-20-1449.md) — process-discussion — `action-state-authority-and-limbo` filed from compiled excerpts of this
  conversation; 5 topics (1 continuing `action-state-architecture`, 4 first mentions).

- [2026-09-20-1447](2026-09-20-1447.md) — session-close — closed the 2026-09-18-1251 session note (09-18 to 09-20 loadout/turn-end
  system, Board Shift direction fix); filed the effective-per-turn-cap decision.

- [2026-09-20-1340](2026-09-20-1340.md) — session — the per-turn cap is now runtime state on both action state structs
  (seeded once, not reset each turn), read by `CanUseAction`, exposed via `GetActionMaxUsesPerTurn` for the Actions UI;
  groundwork for temporary max-uses changes. Builds; no modifier API yet.

- [2026-09-20-1250](2026-09-20-1250.md) — session — Step 2 Stage 3 (remove): legacy `Actions`,
  `bIsRequired`, per-action caps/cooldowns, `RequiredActionTagA/B` and the SWAP budget deleted; Mediator
  gate now mandatory, seeding logged. C++ builds; Blueprint fallout and PIE outstanding.

- [2026-09-20-1240](2026-09-20-1240.md) — session — Board Shift direction inversion root-caused
  (`GetTilesByDirection` grouped Up/Down by the wrong coordinate, so the sort key was 0); fixed
  for X = left-right, Y = up-down. First attempt (table rewrite) superseded same session. Not compiled.

- [2026-09-18-1445](2026-09-18-1445.md) — session — Step 2 Stage 2 (switch): Mediator gates and
  spends per-action uses from PlayerState, server turn-boundary reset/cooldown hooks, limbo in the
  actions component, tree reads PlayerState, `ProducesRequestType` on Place/Swap/BoardShift. Written,
  NOT compiled (editor open).

- [2026-09-18-1510](2026-09-18-1510.md) — session — skin plugin slice 3: generic code-built
  debug panel (`SkinMechanics.ToggleDebug`), `GetSkinResolution` + `GetConfigurationIssues`
  on the subsystem (catalog validation delivered). Decision filed. Not compiled.

- [2026-09-18-1425](2026-09-18-1425.md) — session — Step 2 Stage 1 (additive): replicated
  per-action state + server API + `OnActionRuntimeStateUpdated` on `ATurnBasedPlayerState`,
  `ActionTag` on `FTurnActionRequest`, component builds from config arrays (legacy fallback
  kept), loadout validator. Builds clean; Stages 2 (switch) and 3 (remove) open.

- [2026-09-18-1422](2026-09-18-1422.md) — session — skin plugin slice 2: trimmed to a
  project-agnostic core (deleted the skin/catalog types, kept only `FSkinPresetRowBase`),
  added `USkinSubsystem` + `USkinSaveGame` (precedence resolution, `OnSkinChanged`, runtime-only
  level-forced, save/reload). Decision filed. Not compiled.

- [2026-09-18-1358](2026-09-18-1358.md) — session — skin system slice 1: new plugin
  `UnrealSkinMechanics` (scaffold, `USkinSettings` with DataTable-array catalogs, row/
  selection structs, `USkinDataAsset` base), enabled in the uproject, registered as a vault
  domain; naming/DataTable decision filed. Not built yet (new plugin, editor restart).

- [2026-09-18-1400](2026-09-18-1400.md) — session — action config and turn-end leaves now
  reference `TSubclassOf<UTurnBasedAction>` (tag derived from the class default object via
  `GetTagForClass`) instead of a typed tag; builds clean. Decision filed; Step 2 open.

- [2026-09-18-1350](2026-09-18-1350.md) — session — recorded the owner's `ActionConfig.h`
  changes (numbered actions grantable + `MaxHeldUses`, runtime state split) as a decision;
  fixed stale comments; editor-side test steps for the turn-end tree. Not rebuilt (editor open).

- [2026-09-18-1337](2026-09-18-1337.md) — session — skin-system design recorded (no code):
  4 decision notes (layered data/flow, local-only + fixed-class data, new plugin +
  explicit lists, faction visuals as a curated skin group) + `design/skin-system.md`;
  marked the retired faction-visuals decision partially superseded

- [2026-09-18-1320](2026-09-18-1320.md) — session — loadout/turn-end task slice 1: new
  turn-end requirement tree (Instanced UObject Leaf/Group nodes — the recursive struct
  failed UHT), action-config + runtime-state structs, base `CanAutoEndTurn` evaluates the
  tree when a loadout sets one. Builds clean; not yet PIE-tested, validator and PlayerState
  wiring still open.

- [2026-09-18-0144](2026-09-18-0144.md) — session — implemented the registry-mapping-
  refresh fix: new `UGridDefinition` (grid geometry extracted off the tile registry),
  new `GameEventTaskSubsystem::OnAnyTagComplete`, `PieceRegistry` re-keying `PieceMap`
  from `FConnectItBoardChangeEvent`'s own positions on shift/swap/remove. 3 new
  decisions, 2 code pages updated. Not yet built/PIE-verified.
- [2026-09-17-1615](2026-09-17-1615.md) — session-close — updated the 2026-09-17 session
  note's `## Close` in place, filed the visual-events-in-Blueprint decision, flagged
  piece-tile-tracking as a question candidate
- [2026-09-17-1600](2026-09-17-1600.md) — session — filed
  [`board-shift-loop-visuals`](../_questions/board-shift-loop-visuals.md) as a new open
  question (board-shift wrap-tile identification + visual treatment), per owner's request
- [2026-09-17-1455](2026-09-17-1455.md) — session — backfilled the 09-16 session note's
  `## Close` and wrote a new 09-17 session note, reconstructed from `_logs/` entries and
  real file-modification timestamps (not conversational memory) since `/session-close` and
  `/session-open` were both skipped
- [2026-09-17-1443](2026-09-17-1443.md) — session — started `ConnectIt/design/`
  (deliberately schema-less): notes for Place Piece, SWAP, Board Shift, and a general
  "tiles with powers" feature idea with 5 example powers; `CLAUDE.md` updated to point at
  it
- [2026-09-17-1203](2026-09-17-1203.md) — session — updated `action-gameplay-tags`'s
  `## Current thinking` with the root cause of the `ActionTag` instancing bug (Instanced
  `Loadout.Actions[]` + plain-`EditAnywhere` `ActionTag`, not `DefaultToInstanced`) and two
  candidate fixes, neither applied — owner wants to mull it over
- [2026-09-16-1550](2026-09-16-1550.md) — session — first `/session-open` run; opened
  `ConnectIt/_sessions/` (didn't exist yet) and wrote its first map note; flagged the
  repo-root `.claude/skills/` mirrors as stale/generic (diverged from the vault's authored
  skills) and `CLAUDE.md`'s missing single-next-action field since the 2026-09-14 "Current
  state" removal
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
