---
Date: 2026-09-18
status: Active
superseded by:
tags:
  - action-state
  - turn-end
  - loadout
---
## Decision

Replace `UConnectIt_TurnBasedActionsComponent::RequiredActionTagA`/`RequiredActionTagB`
(and the base plugin's `UTurnBasedActionsComponent::CanAutoEndTurn_Implementation`, which
treats every `bIsRequired` action as mandatory-AND) with a generic, data-driven system.
This belongs in `UnrealTurnBasedMechanics` — nothing about it is ConnectIt-specific.

**Turn-end requirements — a recursive tree, one root per Loadout:**

```cpp
UENUM()
enum class ETurnEndCombineMode : uint8 { Any, All };

USTRUCT()
struct FTurnEndRequirementNode
{
    UPROPERTY(EditAnywhere)
    bool bIsEnabled = true;                      // disabled = skipped, as if absent

    UPROPERTY(EditAnywhere)
    FGameplayTag ActionTag;                      // leaf if ChildGroups is empty

    UPROPERTY(EditAnywhere, meta=(EditCondition="ChildGroups.Num()==0", ClampMin=1))
    int32 RequiredUsesThisTurn = 1;               // only meaningful on a leaf

    UPROPERTY(EditAnywhere)
    TArray<FTurnEndRequirementNode> ChildGroups;  // composite if non-empty

    UPROPERTY(EditAnywhere)
    ETurnEndCombineMode Mode = ETurnEndCombineMode::Any;
};

// one root, on the Loadout asset:
UPROPERTY(EditAnywhere) FTurnEndRequirementNode TurnEndRequirements;
```

**Action config — split by lifecycle, two arrays on the same Loadout asset (a permanent
action's config never becomes a numbered action's mid-match, so a type split beats a bool
discriminator on one struct):**

```cpp
USTRUCT()
struct FPermanentActionConfig
{
    UPROPERTY(EditAnywhere) FGameplayTag ActionTag;
    UPROPERTY(EditAnywhere, ClampMin=0) int32 MaxUsesPerTurn = 0;  // 0 = unlimited
    UPROPERTY(EditAnywhere, ClampMin=0) int32 CooldownTurns = 0;
};

USTRUCT()
struct FNumberedActionConfig
{
    UPROPERTY(EditAnywhere) FGameplayTag ActionTag;
    UPROPERTY(EditAnywhere, ClampMin=1) int32 StartingMatchUses = 1;
    UPROPERTY(EditAnywhere, ClampMin=0) int32 MaxUsesPerTurn = 0;
    UPROPERTY(EditAnywhere, ClampMin=0) int32 CooldownTurns = 0;
};
```

**Runtime state — stays one struct (no permanent/numbered split needed here; see Why),
per action, on `PlayerState`:**

```cpp
USTRUCT()
struct FActionRuntimeState
{
    UPROPERTY() int32 MatchUsesRemaining = 0;    // only meaningful for numbered actions
    UPROPERTY() int32 UsesThisTurn = 0;          // resets every turn; what every leaf's
                                                   // RequiredUsesThisTurn compares against
    UPROPERTY() int32 CooldownTurnsRemaining = 0;
};
```

**New validator, one pass, three checks:** no action tag present in both
`PermanentActions`/`NumberedActions` or in neither while referenced by a tree leaf; every
leaf's `RequiredUsesThisTurn` ≤ that tag's own `MaxUsesPerTurn` when capped (a leaf that
requires more uses than the action can ever reach this turn is structurally unreachable).

## Why

Board Shift ending the turn only after 2 uses is a second real, concrete data point
beyond SWAP/PlacePiece's existing OR-pair — the exact trigger condition
[`_tasks/active.md`](../_tasks/active.md)'s deferred generalization task was waiting on.
It revealed the turn-end model actually needs **two independent axes**, not one: how many
times a single action must complete before *its own* contribution counts (new), and how
multiple actions' contributions combine (the AND/OR-groups need already named in
[2026-09-14 — Turn-end requirements need AND/OR groups](2026-09-14-turn-end-requirements-need-and-or-groups.md)).

The design went through two real corrections worth recording so they aren't silently
re-made:

1. **Config/state conflation.** An early version tried to carry a single
   `bHasMatchUseLimit` bool + one `UsesRemaining` field meaning different things depending
   on that bool — the same "one field, two meanings" bug shape already named in the
   blocker/`FactionPiece` discussion two days prior. Fixed by splitting into designer
   authored *config* (two typed arrays on the Loadout) and per-player *runtime state* (one
   struct, no split needed — its one conditionally-unread field, `MatchUsesRemaining` on a
   permanent action, is inert, not a mismatched-value bug like the config version was).
2. **Threshold ownership.** The first cut put `RequiredUsesThisTurn` (then named
   `RequiredUsesPerTurnForAutoEnd`) on the action's own config — a single global value.
   Broke on a concrete case: Group 1 = "Place ×2 alone ends the turn," Group 2 = "Place ×1
   AND Swap ×1 ends the turn." A shared global threshold on Place can only be one number —
   set to 2 for Group 1's sake and Group 2 becomes permanently unreachable, since "has
   Place met its threshold" then always means "≥2" everywhere, including inside Group 2
   which only ever needed 1. Fixed by moving the threshold onto the tree leaf itself —
   the same action can be referenced by multiple leaves, each with its own independent
   threshold, all reading the same underlying `UsesThisTurn` runtime counter.
   `bContributesToAutoEndTurn` became redundant once leaf-presence-in-the-tree is the
   signal for "this counts" — replaced by `bIsEnabled` (skip semantics) purely as a
   designer convenience to disable a leaf/subtree for testing without deleting it.

Both PlacePiece's and Board Shift's real requirements now map cleanly onto this shape —
see the [2026-09-18 check-in](../../Development/optimal-co-developer/_meetings/2026-09-18-check-in.md)
transcript for the full derivation and every example case walked through it.

## What Would Change It

Two things were explicitly raised and scoped out, not designed: **prerequisite actions**
(an action only usable after another has completed) and a **mid-match-granted special
numbered action** feeding into a group-of-groups OR. Both are named as real near-term
possibilities, not hypotheticals invented to stress-test the shape — if either becomes
concrete, this tree/config pair is the thing to extend, not replace.
