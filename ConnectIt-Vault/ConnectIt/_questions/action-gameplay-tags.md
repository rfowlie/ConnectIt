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

**2026-09-17 — root cause found for one of the four places (the instance `ActionTag`).**

`Loadout->Actions[]` ([`ActionLoadoutDataAsset.h`](../../Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/ActionLoadoutDataAsset.h))
is `UPROPERTY(EditAnywhere, Instanced, ...)` — every entry placed into it becomes its own
fully-serialized sub-object, a deep copy, not a reference. That's correct and wanted for
this array (same mechanism that gives each controller its own independent runtime clone
later). The bug is that `ActionTag`, declared on `UTurnBasedActionBase`
([`TurnBasedActionBase.h`](../../Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/TurnBasedActionBase.h)),
is plain `EditAnywhere` — so it rides along as just another per-instance-editable field.
The moment an action is placed into `Loadout.Actions[]`, that placement gets its own
independent copy of `ActionTag`, seeded from the class's CDO at that moment; changing the
class's own defaults afterward doesn't propagate to the already-placed instance. That's the
"set it in two places" experience — the class defaults' `ActionTag` and the specific
loadout placement's `ActionTag` are two independently-editable copies with nothing keeping
them in sync.

**Not caused by `DefaultToInstanced`** on the action classes, and removing it wouldn't fix
this — `Actions[]` already says `Instanced` explicitly regardless of the class specifier,
and `DefaultToInstanced` is what correctly makes an *unmarked* future `UObject*` property of
this type default to owned/duplicated behavior (the same mechanism the per-controller
runtime-clone pattern already depends on elsewhere). Removing it would only remove a safety
net, not touch this bug.

**Candidate fix, not yet applied (holding off per owner):** change `ActionTag` from
`EditAnywhere` to `EditDefaultsOnly` — still settable at the class level (C++ constructor,
or a Blueprint child's Class Defaults), but no longer offered as a per-placement override
inside `Loadout.Actions[]`'s inline instanced-object editor, so every instance just
inherits the one value the class defines. Plain C++ `protected`/`private` was considered
and ruled out as the mechanism — access level doesn't control editor exposure in UE's
reflection system, the UPROPERTY specifier does. A stronger version worth weighing
alongside it: drop `ActionTag` as a `UPROPERTY` entirely and derive it in code (a
`virtual FGameplayTag GetActionTag() const` each concrete action class hardcodes) — closes
the loop completely, no data field left to diverge at all.

**Still separate, not addressed by the above:**
`UConnectIt_TurnBasedActionsComponent::RequiredActionTagA`/`RequiredActionTagB` — a plain
`FGameplayTag` on a different object entirely (the ActionsComponent, not the action), hand-
typed to match SWAP/PlacePiece's `ActionTag` by a human reading two places. No Instanced-
object mechanics involved, so the fix above won't touch it. Candidate direction floated:
move "does completing this action satisfy the OR-group" onto the action instance itself
(e.g. a `bSatisfiesAutoEndTurn` bool next to `bIsRequired`) so `CanAutoEndTurn_Implementation`
checks a flag on each action instead of tag-matching against a separately-typed pair on the
component — would also double as a natural stepping stone toward the AND/OR-groups
generalization already deferred in
[`_decisions/2026-09-14-turn-end-requirements-need-and-or-groups.md`](../_decisions/2026-09-14-turn-end-requirements-need-and-or-groups.md).

Owner wants to mull over both candidate directions before committing — nothing implemented
yet.
