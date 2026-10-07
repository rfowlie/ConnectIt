# Live match setup

Design + plan note (2026-10-07). Approved in plan mode.

## Context
`UConnectIt_GameUtilityLibrary::GetLevelConfig` hands anyone the level's *starting template*. Live values already differ
(the menu's target score and AI profile overrides) and will differ more (rules that change mid-level, a player's actions
changing). Today each client's player controller even builds its action stack straight from
`LevelConfig->PlayerLoadout`. Owner's direction: the GameMode acquires the level config, owns the live copies, and
everyone else reads live data through accessors. Because the GameMode exists only on the server, agreed shape:
**GameMode = single authority and only reader of the config's live fields; match-wide values mirror to the GameState
(replicated); per-player values live on PlayerStates (already replicated); fixed level setup stays a direct asset read
through narrow accessors.** This also delivers the "rules visible to clients" half of
`design/mid-level-rule-changes.md`.

## Changes

### 1. GameMode resolves and owns the match setup (`AConnectIt_GameMode`)
- `EnsureMatchSetupResolved()` (idempotent; called from `PostLogin` and `HandleMatchHasStarted`, since players log in
  before the match starts): resolves once -- `Rules` (config copy + menu target score), `PlayerLoadout`, `AIProfile`
  (menu choice → config default). The existing resolution code moves here.
- Accessors: `GetRules()` (exists), `GetPlayerLoadout()`, `GetAIProfile()`.
- **One entry point for changing rules:** `ModifyRules(TFunctionRef<void(FConnectItRuleSet&)>)` applies the change to
  the GameMode's copy, then publishes the mirror. Used for the menu override now; mid-level rule changes later.
- `SeedActionStateForPlayer` uses `GetPlayerLoadout()`; `AConnectIt_AIController::ResolveAIProfile` asks the GameMode
  (the AI stops reading the config).

### 2. GameState mirror (`AConnectIt_GameState`)
- Replicated `FConnectItRuleSet MatchRules` (`ReplicatedUsing` → `OnMatchRulesChanged` event, also fired on the server)
  and replicated `TObjectPtr<UConnectIt_AIProfile> OpponentProfile`; server-only setters called by the GameMode.
- Blueprint-pure readers: `GetMatchRules()`, `GetOpponentProfile()`.

### 3. Clients build actions from their PlayerState, not the config
- Plugin `ATurnBasedPlayerState`: `Loadout` becomes `ReplicatedUsing = OnRep_Loadout`; new `OnLoadoutChanged` delegate
  (fired from `OnRep_Loadout` and from `InitialiseActionState` on the server).
- Plugin `UTurnBasedActionsComponent::InitialiseFromLoadout`: remove its server-side PlayerState seeding block (the
  GameMode is the one seeder for humans, the AI controller for itself) -- otherwise building from the PlayerState would
  re-seed it in a loop.
- `AConnectIt_PlayerController`: replace `InitialiseFromLevelConfig` with `InitialiseActionsFromPlayerState` -- for the
  local controller, bind `OnLoadoutChanged` (in `BeginPlay` if the PlayerState exists, else in `OnRep_PlayerState`) and
  call `ActionsComponent->InitialiseFromLoadout(PS->GetLoadout())` when a loadout is present / changes. A later change
  re-initialises (logged); swapping mid-turn is untested until a feature needs it.

### 4. Narrow accessors instead of the whole asset
- Remove `UConnectIt_GameUtilityLibrary::GetLevelConfig` (public, Blueprint-callable). The lookup moves to
  `UConnectIt_LevelConfigSettings::FindLevelConfig(WorldContext)` -- C++ only, documented as "starting template: for the
  server's GameMode and world bootstrap".
- Fixed-setup accessors on the utility library (Blueprint-pure): `GetPieceActorClass`, `GetPiecePoolInitialSize`
  (the registries and grid definition are already reached through `UConnectIt_BoardRegistrySubsystem`, which keeps
  reading the templates at world start, before anything replicates).
- Live accessors on the utility library, forwarding to the GameState / PlayerState: `GetMatchRules`,
  `GetOpponentProfile`, `GetLocalPlayerLoadout`.

## Files
`Source/ConnectIt/{Public,Private}/Framework/GameMode/ConnectIt_GameMode.*`, `.../GameState/ConnectIt_GameState.*`,
`.../Controller/ConnectIt_PlayerController.h` + `AConnectIt_PlayerController.cpp`, `.../Controller/ConnectIt_AIController.cpp`,
`.../Library/ConnectIt_GameUtilityLibrary.*`, `.../Data/ConnectIt_LevelConfigSettings.h` (+ new `.cpp`),
`.../Subsystem/ConnectIt_BoardRegistrySubsystem.cpp`; plugin
`UnrealTurnBasedMechanics/.../Framework/PlayerState/TurnBasedPlayerState.*`, `.../Action/TurnBasedActionsComponent.cpp`.

## Editor fallout (owner)
- `Content/_ConnectIt/_Framework/GameMode/ConnectIt_GameMode_MainMenu` calls `GetLevelConfig` -- replace with the new
  accessors or the level catalog.

## Vault
Step 0: plan moved here from the plan-mode staging file (done); decision `2026-10-07-gamemode-owns-live-match-setup.md`; update `design/mid-level-rule-changes.md` (mirror
now exists; what remains is allowed-axes + the trigger); log + index. Task rows left to the owner.

## Verification
- Build (editor closed); `Automation RunTests ConnectIt.AI` still 9/9.
- I can't test replication headless -- owner, PIE with **2 players (listen server + client)**: both players' action
  UI appears and works (log: `seeded action state` once per player on the server, then
  `initialised actions from PlayerState loadout` on each local controller); the client's GameState has the rules
  (`GetMatchRules` shows the target score, including a menu override); vs AI still plays.

## Status (2026-10-07) -- implemented, built; replication not yet verified

- Build clean; 9/9 `ConnectIt.AI` tests pass. Decision:
  [2026-10-07-gamemode-owns-live-match-setup](../_decisions/2026-10-07-gamemode-owns-live-match-setup.md).
- **Needs the owner's 2-player PIE pass** (listen server + client) -- replication can't be tested headless. Expected
  log: `match setup resolved` once on the server; `seeded action state` once per player; `initialised actions from
  PlayerState loadout` on each local controller; both players' action UI working; `GetMatchRules` on the client showing
  the target score.
- Editor: `ConnectIt_GameMode_MainMenu` still calls the removed `GetLevelConfig` node.
