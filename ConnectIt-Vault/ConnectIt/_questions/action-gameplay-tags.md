---
created: 2026-09-14
question: "What do we do about action gameplay tags being set in 4-5 separate places for the system to function correctly?"
status: open
closed-by:
tags:
  - question
---

# What do we do about action gameplay tags being set in 4-5 separate places for the system to function correctly?

## Why it matters

Getting a new action working today means setting what's effectively the same identity
(which action this is) in several separate, disconnected places, with nothing enforcing
that they agree:

- `Config/DefaultGameplayTags.ini` — the UI-facing `ConnectIt.Action.*` tag declaration
  (e.g. `ConnectIt.Action.PlacePiece`, `ConnectIt.Action.SwapPiece`).
- The action instance's own `ActionTag` property, hand-set per instance in the level
  loadout asset's `Actions[]` array in the editor.
- The native, compiled wire-level tag (`ConnectIt_Game_PlacePiece`,
  `ConnectIt_Game_SwapPieces` in `ConnectIt_GameplayTags.h`/`.cpp`) used for
  `FTurnActionRequest::RequestType` dispatch in
  `UConnectIt_BoardRequestMediator::ProcessRequest`.
- `UConnectIt_TurnBasedActionsComponent::RequiredActionTagA`/`RequiredActionTagB` — has to
  be hand-set (Class Defaults) to match the same action's `ActionTag`, or the
  PlacePiece/SWAP alternate-turn-end logic silently stops recognizing that action.

Already a confirmed source of real bugs this session, not a hypothetical: PlacePiece's
`HandleValidSelection_Implementation` sets `Request.RequestType = ActionTag;` — which only
dispatches correctly today because someone hand-set the editor `ActionTag` value to the
*native* tag string, silently diverging from the clean ini-declared value. That's tracked
as its own fix in [`ConnectIt/_tasks/active.md`](../_tasks/active.md) ("Fix
`ConnectIt_PlacePieceAction`'s `ActionTag`/`RequestType` conflation"); this question is
the broader version of the same problem — that fix addresses one action's one symptom,
not the general pattern that made it possible.

## What would answer it

A design that gets action identity down to one authored place a designer/programmer sets
once per action, with everything else (UI display, wire dispatch, turn-end-pair
membership, whatever else keys off "which action is this") reading from that single
source instead of being separately, manually kept in sync.

## Current thinking

Not started — flagged during SWAP debugging, not yet designed.
