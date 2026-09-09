---
schema: code
kind: UCLASS
role: primary
source:
  - Source/ConnectIt/Public/Board/ConnectIt_BoardStateComponent.h
  - Source/ConnectIt/Private/Board/ConnectIt_BoardStateComponent.cpp
  - Source/ConnectIt/Public/ConnectIt_Structs.h
reconciled: 2026-09-07
commit: 668872e
---

# UConnectIt_BoardStateComponent

`UBoardStateComponentBase` subclass on
[[game/code/AConnectIt_GameState|AConnectIt_GameState]]. **The board's single source of
truth.** Exactly one replicated property — `FConnectItBoardStateSnapshot BoardSnapshot`
(previous + current `FConnectItBoardState` + the `FConnectItBoardChangeEvent` describing
the delta) — so state and "what changed" arrive atomically, and one signal drives every
visual system on both server and client.

## When you touch this

- Any authoritative board mutation (server).
- Reading current / previous board state, or the last change event.

## Entry points

- **Server (board request handlers only):**
  `InitialiseBoardState(TileRegistry, PieceRegistry, NumFactions, InitialMultiplier,
  InTargetScore)` — once at game start; stamps `TargetScore` up front.
  `SetBoardState(NewState, ChangeEvent)` — captures current→previous, applies new, stores
  the change event, fires `OnBoardStateChanged` on server, then `EnqueueBoardEventTags()`.
- **Reads:** `GetCurrentState()`, `GetPreviousState()`, `GetChangeEvent()` (payload for
  zero-param board-event-tag listeners), `GetBoardSnapshot()`, `GetInfo()` →
  `FConnectItBoardStateInfo`.
- `OnBoardStateChanged` (inherited, **zero-param**) — bind + then read
  `GetCurrentState()` / `GetChangeEvent()`.
- Key struct helpers on `FConnectItBoardState`: `GetTileData` / `GetTileDataMutable` /
  `SetTileData` (parallel `TilePositions` / `TileDataArray` — a `TMap` can't replicate),
  `IsTileValidForPlacement` (= active && !occupied), `GetScore`, `NumTiles`.

## Collaborators

- `OnRep_BoardSnapshot` (client) and `SetBoardState` (server) **both** call
  `BroadcastChange()` then `EnqueueBoardEventTags()` — symmetric.
- `EnqueueBoardEventTags()` reads `ChangeEvent` and fires
  [[UnrealGameMechanics/code/UGameEventTaskSubsystem|UGameEventTaskSubsystem]]
  `QueueTagContainer` once per event, fixed order: shift/piece-placed → line-scored →
  player-win.
- Written by
  [[game/code/UConnectIt_BoardRequestMediator|UConnectIt_BoardRequestMediator]] handlers
  via `SetBoardState`.

## Gotchas

- **Only `SetBoardState` may mutate** — never write `CurrentState` directly.
- `FConnectItBoardChangeEvent::bGameWon` is **edge-triggered** (true only on the
  transition into game-over); `FConnectItBoardState::bGameOver` stays true afterward.
- `EnqueueBoardEventTags` runs on both machines — visual sequencing must be
  client-safe/idempotent.
- Scoring/win happen *before* `SetBoardState` (the mediator mutates a working
  `FConnectItBoardState`, runs rules on it, then commits) — the change event already
  carries `bLineScored` / `bGameWon`.
- Ported from a now-deprecated `UConnectIt_BoardSequencerComponent` (which derived the
  tag list from a separate listener).

## Cross-impact

Adding a change kind ⇒ new `FConnectItBoardChangeEvent` fields + an
`EnqueueBoardEventTags` branch + the matching gameplay tag + a `HandleXRequest` that sets
the fields. UI reading the snapshot (`AConnectIt_GameState` wrappers, debug widgets,
`UConnectIt_BoardStateLibrary`) must handle the new fields.

## See also

- In-repo: `old/Source/ConnectIt/Docs/Workflows/SingleSourceOfTruth-Replication.md`;
  `old/Source/ConnectIt/Docs/Workflows/GameplayTag-EventSequencing.md`; `old/Source/ConnectIt/Docs/UIValueCatalogue.md`.
- [[game/systems/board-state-single-source-of-truth|systems/board-state-single-source-of-truth]] ·
  [[game/code/ConnectItStructs|ConnectItStructs]]
