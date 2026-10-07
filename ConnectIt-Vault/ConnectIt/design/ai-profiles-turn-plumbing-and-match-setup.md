# AI profiles, AI turn plumbing, and match setup

Design note (2026-10-02), agreed in discussion after the Classic MinMax AI was confirmed working in PIE. Not yet
implemented. Three workstreams; suggested order below.

## 1. AI profile assets (agreed: generic profile asset)

- `UConnectIt_AIProfile` (`UDataAsset`): `DisplayName`, `Description` (for the menu), and an `Instanced`
  `TObjectPtr<UConnectIt_AIStrategy> Strategy`. Presets are assets (e.g. `AIP_Classic_Easy`, `AIP_Classic_Hard`).
- One asset type for every strategy -- no paired data-asset class per strategy (that would re-create the parallel
  hierarchy removed on 10-02). A strategy's own properties are its configuration; the asset just stores a configured
  instance.
- `UConnectIt_LevelConfigDataAsset::AIStrategy` → `AIProfile` (soft or hard ref). The controller keeps duplicating the
  strategy (`DuplicateObject(Profile->Strategy, Controller)`) per match.
- Open: whether the AI's loadout also moves onto the profile (an AI is then fully described by one asset) or stays
  `EnemyLoadout` on the level config.

## 2. AI turn plumbing without an actions component (agreed: option C -- the controller does it)

Trade-offs discussed: A (keep the actions component) is zero work but forces human-only loadout setup (mandatory
`RootActionClass`, viewer actions) and builds an unused stack; B (slim plugin AI component) is the most reusable but
touches the human path via the coordinator; **C** chosen: fewest moving parts, all AI turn handling readable in one
place. Accepted cost: AI turn plumbing lives in ConnectIt, not the plugin.

- Plugin `ATurnBasedAIController`: stop creating `ActionsComponent` and `CoordinatorComponent` (keep
  `ParticipantComponent` + PlayerState creation). Human controllers unchanged.
- Shared refactor: `ATurnBasedPlayerState::CanEndTurn()` -- evaluates the loadout's `TurnEndRequirements` via the
  existing `UTurnEndRequirementNode::IsSatisfied` with the PlayerState's own per-action use counts; no tree = true.
  `UTurnBasedActionsComponent::CanAutoEndTurn_Implementation` calls it (behaviour unchanged for humans).
- `AConnectIt_AIController`:
  - seeds its PlayerState directly: `PS->InitialiseActionState(EnemyLoadout)` (as the GameMode does for humans);
  - binds `ParticipantComponent->OnTurnNotificationReceived_Native`; starts a decision on `TurnStart`/`TurnActive`
    **once per turn number** (the coordinator currently fires "turn started" for both phases, which restarts the
    AI's search); cancels on `TurnEnd`/`TurnTimeout`/`TurnSkipped`;
  - binds `GameState->OnMatchPhaseChanged_Native`: cancel on `Paused`/`GameOver`, re-decide on resume if still its turn;
  - turn end: `PS->CanEndTurn()` then `ParticipantComponent->ServerSubmitTurnEnd()`.
- Delete `UConnectIt_AIActionsComponent` and the `FObjectInitializer` subobject swap. Ready check needs nothing:
  the participant manager already marks AI participants ready.
- Enemy loadouts then only need action configs + turn-end tree (no system actions).

## 3. Main-menu match setup (agreed: all four pieces)

- `UConnectIt_MatchSetupSubsystem` (`UGameInstanceSubsystem`): holds `FConnectItMatchSettings` -- level, AI profile,
  target score (0 = level default); BlueprintCallable setters for the menu; survives `OpenLevel`.
- Level catalog: a data asset listing playable levels (display name, map, thumbnail, optional default AI profile);
  the menu lists it and calls `OpenLevel`.
- AI profile picker: the menu lists `UConnectIt_AIProfile` assets (or a list on the catalog); the chosen one
  overrides the level config's.
- `AConnectIt_GameMode` reads the subsystem at match start: target score → win condition, AI profile → AI controller.
  Keep one GameMode class with `MatchType` (Adventure = vs AI) rather than a new VersusAI GameMode class.
- **Prerequisite bug fix:** `HandleMatchHasStarted` assigns the level config's rule objects directly
  (`BoardRules->WinConditionRule = LevelConfig->WinConditionRule`, same for scoring/placeable). They are subobjects
  of the shared data asset, so overriding the target score would modify the asset (and leak across PIE sessions).
  Duplicate them per match, as the registries and the AI strategy already are.
- Later, for online: write the same settings as URL options (`?TargetScore=`) and parse them in `InitGame`.

## Suggested order

1. Rule duplication fix (small, independent, unblocks 3).
2. AI turn plumbing (C) -- removes the subclass trick before more AI code builds on it.
3. AI profile asset.
4. Match setup subsystem → target score override → level catalog + picker → AI profile picker (UMG work is the owner's).

## Status (2026-10-02) — implemented, built; PIE not yet re-run

All four steps done in the suggested order; the AI loadout moved onto the profile (owner's call). Build clean; the 7
`ConnectIt.AI` tests pass. Decisions:
[profile asset](../_decisions/2026-10-02-ai-opponent-is-a-profile-asset.md),
[AI runs its own turn](../_decisions/2026-10-02-ai-controller-runs-its-own-turn.md),
[match setup subsystem](../_decisions/2026-10-02-main-menu-match-setup-subsystem.md).

- **Expected data loss on update:** `EnemyLoadout` and `AIStrategy` were removed from the level config -- recreate them
  as a `UConnectIt_AIProfile` asset and set `AIProfile` on the level config.
- Also fixed on the way: `GI_MinMaxAlphaBeta.h` line 140 read `for (... : 11)` (committed in `febb550`), restored to
  `RootMoves` with the owner's OK.
- Menu UI (UMG) is the owner's: read `GetLevelCatalog()`, build `FConnectItMatchSettings`, call `StartMatch`.
