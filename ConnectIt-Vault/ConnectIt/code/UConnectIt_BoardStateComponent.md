---
schema: code
kind: UCLASS
role: primary
source:
  - Source/ConnectIt/Public/Board/ConnectIt_BoardStateComponent.h
  - Source/ConnectIt/Private/Board/ConnectIt_BoardStateComponent.cpp
  - Source/ConnectIt/Public/ConnectIt_Structs.h
reconciled: 2026-09-14
commit: 131609f
---

# UConnectIt_BoardStateComponent

`UBoardStateComponentBase` subclass on
[[AConnectIt_GameState|AConnectIt_GameState]]. **The board's single source of
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
  `QueueTagContainer` once per flag set on that event — each call represents exactly one
  concrete mutation kind, so at most one of the "concrete change" tags fires per call:
  `bPiecePlaced` → `ConnectIt_Event_PiecePlaced`, `bPieceRemoved` → `…PieceRemoved`,
  `bPiecesSwapped` → `…PiecesSwapped`, `bPieceCaptured` → `…PieceCaptured`,
  `bTileMultiplierDestroyed` → `…TileMultiplierDestroyed`, `bTileActiveToggled` →
  `…TileActiveToggled` (this last one has no ordering relationship with the others — a
  disjoint kind of change). Then, independently, the two "knock-on" tags:
  `bLineScored` → `…LineScored`, `bGameWon` → `…PlayerWin`.
- Written by
  [[UConnectIt_BoardRequestMediator|UConnectIt_BoardRequestMediator]] handlers
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

## Changes

- 2026-09-14 — corrected the Collaborators event-tag list: it previously described a
  three-tag "shift/piece-placed → line-scored → player-win" fixed order that no longer
  matches the source — six independent concrete-change tags now exist (including
  `bPiecesSwapped`/`bPieceCaptured` from the SWAP/capture work), each firing from its own
  disjoint `ChangeEvent` flag.
- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[ConnectIt/CLAUDE|ConnectIt overview]].
- [[board-state-single-source-of-truth|systems/board-state-single-source-of-truth]] Â·
  [[ConnectItStructs|ConnectItStructs]]
