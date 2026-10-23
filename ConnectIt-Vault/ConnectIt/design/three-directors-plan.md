---
type: plan
status: complete
approved: true
approved-on: 2026-10-20
completed-on: 2026-10-21
playtested: false
design: "[three-directors](three-directors.md)"
question: "[level-trigger-board-changes](../_questions/level-trigger-board-changes.md)"
decision: "[2026-10-21-director-hub-and-level-changes](../_decisions/2026-10-21-director-hub-and-level-changes.md)"
---

# Plan: three directors, lose conditions, level triggers and level changes (part 2)

Design: [three-directors](three-directors.md). Part 1 (built): [conditions-and-legal-moves](conditions-and-legal-moves.md).
Written 2026-10-20 after the open-points discussion; approved and **built** 2026-10-20..21 (47/47 tests). Log:
[_logs/2026-10-20-2330](../_logs/2026-10-20-2330.md).

## Context

Part 2 turns the single AI director into three directors -- Level, Player, AI -- on a shared base, moves the match's
conditions into them, adds lose conditions and level triggers, and gives the Level director Blueprint ways to change the
board and the scoring rule. Settled in the 2026-10-20 discussion (the last four confirmed when the owner asked for this plan):

- Conditions are **authored in directors, evaluated in the rule set** (copied at match start) -- except **level triggers**,
  which the Level director checks itself after each committed board change and keeps their fired / armed state.
- **Event order** when several directors react: Level, then Player directors by slot, then AI. Directors find each other
  through the GameMode; only a side's director changes that side's loadout; the Level director changes the board and the rules.
- **Remove the GameMode's `On Win Checkpoint Reached` / `On Match Won` delegates** (nothing uses them; directors replace them).
- **Migration by checklist**, not code: two presets, values noted before the build.
- Lose conditions: any one holding = that side loses; win checked first; `Lose Condition Met` event then Game Won; an
  `Opponent Lost` end reason; the search treats a loss as game over; **AI matches only** (online undecided).
- Rule changes: a Blueprint-callable scoring-rule change owned by the Level director, applied at a safe moment.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between stages;
nothing is committed by me.

## Stage 1: director base and the three kinds; the preset names classes

- **`UConnectIt_Director`** (new, abstract, `Public/AI/ConnectIt_Director.h`): world access; **events** with "is it mine"
  (`OnMatchStarted`; `OnTurnStarted(Faction, bIsMine, TurnNumber)`; `OnTurnEnded(...)`; `OnWinCheckpointReached(Faction,
  bIsMine, Goal, GroupIndex, GroupName)`; `OnMatchWon(Faction, bIsMine)`); getters for the other directors (through the
  GameMode). "Mine" = the director's own faction (Player / AI); always false for the Level director.
- **`UConnectIt_AIDirector`** now derives from it: keeps its setup (loadout, strategies, selector, think times, pending changes)
  and loadout calls; `OnAITurnStarted` / `OnAITurnEnded` / `OnOpponentTurnStarted` become the base turn events (no Blueprint
  directors exist yet, so nothing to rewire).
- **`UConnectIt_PlayerDirector`** (new): `StartingLoadout` (replaces the preset's `PlayerLoadout`), `SetPlayerLoadout(Loadout,
  Timing)` / `GetPlayerLoadout` for its own player (a new per-slot `SetLoadoutForSlot` on the GameMode beside the role-based
  `SetLoadout`). One instance per human player, outer = GameMode; versus = two instances of the same class.
- **`UConnectIt_LevelDirector`** (new): the level's script. One instance, outer = GameMode. (Triggers and changes: stages 4-5.)
- **The match preset**: `LevelDirectorClass`, `PlayerDirectorClass`, `AIDirectorClass` (`TSubclassOf`) replace the inline
  `AIDirector` and `PlayerLoadout`; `Rules` keeps the scoring rule. Validation validates each class's defaults.
- **The GameMode as the hub**: creates the Level director and a Player director per human (PostLogin / match start; humans
  seeded from the Player director's `StartingLoadout`); the AI controller creates its AI director from the class and
  registers it. The GameMode dispatches, in order Level -> Players by slot -> AI:
  - match start; turn start / end (from the participant manager's turn-phase changes, server-side);
  - checkpoint and match won (from the committed board change, `OnBoardStateChanged` -- state, not the visual queue).
  The AI controller stops dispatching director events itself; it keeps its decision loop, pending changes and the AI pass.
- **Removed**: the GameMode's `OnWinCheckpointReached` / `OnMatchWon` delegates and the checkpoint tag binding
  (`HandleWinCheckpoint`); game-over handling (match result) stays.
- Tests: director events reach all three kinds in order with the right "mine"; a preset with three classes validates; the
  existing director tests ported.

## Stage 2: conditions move into the directors

- `UConnectIt_PlayerDirector` / `UConnectIt_AIDirector`: `WinConditions` (ordered condition assets). The rule set's authored
  `PlayerWinConditions` / `AIWinConditions` are removed; `ResolveConditionGroups` takes the lists from the directors' classes
  (the GameMode calls it at match setup); the runtime groups are unchanged, so the search and clients are untouched.
- Validation moves with them: a director validates its lists (empty entries, empty assets, conditions' own warnings with the
  preset's rules when validated through the preset).
- The GameMode's "no win conditions" warnings read the directors.
- Tests: resolving from directors fills the same groups as before; the existing condition-asset test ported.

## Stage 3: lose conditions

- Player / AI directors: `LoseConditions` (condition assets, any one = loss). Rule set: runtime `PlayerLoseGroups` /
  `AILoseGroups`, `GetLoseGroupsFor` (**empty when there is no AI** -- online ignores them, with a warning), `GetLosingFaction`;
  `GetWinningFaction` = wins first, else the other faction of a loser (so MinMax treats a loss as terminal automatically).
- `StampWinState`: on a loss, `FConnectItBoardEvent_LoseConditionMet {Faction, Asset, GroupName}` (new tag
  `ConnectIt.Event.LoseConditionMet`) and `FConnectItBoardState::LosingFactionSlot`; then Game Won as usual.
- Match result: `EMatchEndReason::OpponentLost` when `LosingFactionSlot` is set.
- Directors: `OnLoseConditionMet(Faction, bIsMine, Condition)` (before `OnMatchWon`).
- Tests: a lose condition ends the match for the other side with the event and slot; win beats loss in the same move; no AI =
  ignored; MinMax `IsTerminalState` on a lost position.

## Stage 4: level triggers

- `UConnectIt_LevelDirector`: `Triggers` -- entries `{ Condition (asset), Side (Player / AI / Either) }`. After each committed
  board change (server), every armed trigger is checked with the match's rules (same condition context); the first time it
  holds it **fires once**: `OnTriggerFired(Condition, Faction)` (Level director only), then disarms. `RearmTrigger(Condition)`
  arms it again. Fired / armed state lives on the director. Not in the rule set; not seen by the AI's search.
- Tests: fires once, not again until re-armed; Side filters; checked against committed state.

## Stage 5: level changes -- board operations and the scoring rule

- **Timing**: `EConnectItLoadoutTiming` becomes `EConnectItChangeTiming` (Next Turn Start / After This Move) with an
  `EnumRedirects` entry; "Immediately" while a move is being carried out becomes "after this move" (never mid-scoring).
- **Board operations outside a move**: the Mediator's board step (CanApply -> CarryOutOperation -> StampWinState -> Game Won ->
  commit with events) is factored out of `DispatchRequest`; a new server path `ProcessLevelOperation(Operation, AsFaction)` runs
  it without the loadout gate or use consumption, deferred until the current move finishes. Level director:
  `ApplyBoardOperation(Operation, AsFaction, Timing)` (`Make Instanced Struct`, e.g. Force Place Piece, Toggle Tile Active,
  Destroy Tile Multiplier). Its events reach visuals like a move's; a win it causes counts.
- **The scoring rule**: GameMode `SetScoringRule(ScoringRule, Timing)` (validated to be a scoring rule struct) through
  `ModifyRules`; Level director `SetScoringRule(...)` / `GetRules()`. Clients see it via the replicated rules and `On Match
  Rules Changed`; the AI from its next decision.
- Tests: a level operation applies and scores without a loadout; deferred while a move is in progress; the scoring rule change
  is seen by the next move and the rules mirror.

## Stage 6: vault

- Recipes: `set-up-a-custom-ai-opponent` and `direct-the-ai-during-a-match` (director classes, base events, Player director);
  `add-a-win-condition` (conditions live in directors; lose conditions); new `script-a-level` (Level director: triggers, board
  operations, scoring-rule changes, finding the other directors); `code/systems/ai-turn.md`; the recipes index.
- `design/three-directors.md` status; `design/mid-level-rule-changes.md` update; close or update
  `_questions/level-trigger-board-changes.md`; decision note (and index); log.

## Owner-side migration checklist (note values before the Stage 1 build)

- `CIMP_Puzzle_Swap_01`, `CIMP_Test`: note the inline AI director's settings (Starting Loadout, Strategies + their settings,
  Selector, think times), `Player Loadout`, and `Rules > Player / AI Win Conditions`.
- After Stage 1: create director Blueprints (e.g. `CIAD_` / `CIPD_` / `CILD_` per level; shared ones for Test / Classic), put
  the noted values in their Class Defaults, set the preset's three director classes.
- After Stage 2: list the `CIWC_*` assets in the Player / AI directors' Win Conditions.
- After Stage 3: exhaustion levels -- a lose asset (Not Enough Pieces To Score / No Legal Move: Swap Pieces) in the Player
  director's Lose Conditions.
- `CI_PieceVisualHandler` and other visuals need nothing: board events are unchanged (plus the new Lose Condition Met).

## Critical files

- New: `Public/AI/ConnectIt_Director.h` + cpp, `ConnectIt_PlayerDirector.h` + cpp, `ConnectIt_LevelDirector.h` + cpp.
- `Public/AI/ConnectIt_AIDirector.h` + cpp, `Private/Framework/Controller/ConnectIt_AIController.cpp`.
- `Public/Framework/GameMode/ConnectIt_GameMode.h` + cpp, `Public/Framework/Data/ConnectIt_MatchPreset.h` + cpp.
- `Public/Board/Rules/ConnectIt_RuleSet.h` + cpp, `Public/ConnectIt_Structs.h` (board state), `Public/Board/Events/ConnectIt_BoardEvents.h`,
  `ConnectIt_GameplayTags.h/.cpp`, `Public/Framework/GameState/ConnectIt_GameState.h` (end reason).
- `Public/Board/ConnectIt_BoardRequestMediator.h` + cpp.
- `Config/DefaultEngine.ini` (enum redirect). Tests: `Private/Tests/ConnectIt_MinMaxTests.cpp`, `ConnectIt_MinMaxTestTerms.h`.

## Verification

- Each stage: build (editor closed) and `Automation RunTests ConnectIt`; all pass, new ones included.
- Owner PIE after the asset work: both levels load with their directors; the AI plays and passes; a checkpoint reaches the
  directors (Level, Player, AI order in the log); the swap level's stuck player loses with `Lose Condition Met`; a level
  trigger fires once; a Level director board operation and scoring-rule change show on the client.

## Risks / notes

- Stage 1 drops the presets' inline director and player loadout (checklist above).
- The hub moves director dispatch from the AI controller to the GameMode: turn-event timing is checked against the AI's
  pending-loadout handling (a loadout set in the AI's turn-end event still waits for the turn to end).
- Level operations change the board outside a player's move: they run only after the current move finishes, and only on the
  server.
