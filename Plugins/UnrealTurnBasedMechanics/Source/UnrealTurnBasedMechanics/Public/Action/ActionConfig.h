// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionConfig.generated.h"

class UTurnBasedAction;

// Designer-authored config for an action that is available for the whole
// match (PlacePiece-style). Split from FNumberedActionConfig by type rather
// than a bool discriminator: an action's lifecycle never flips mid-match, so
// a shared struct would only carry a conditionally-dead field.
USTRUCT(BlueprintType)
struct UNREALTURNBASEDMECHANICS_API FPermanentActionConfig
{
    GENERATED_BODY()

    // The action this config applies to. Its tag is derived from the class
    // (UTurnBasedAction::GetTagForClass), never typed here.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Config")
    TSubclassOf<UTurnBasedAction> ActionClass;

    // 0 = unlimited
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Config", meta = (ClampMin = 0))
    int32 MaxUsesPerTurn = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Config", meta = (ClampMin = 0))
    int32 CooldownTurns = 0;
};

// Per-player, per-action live state for a permanent action -- what
// PlayerState will hold once the loadout reference lands there. Split from
// FNumberedActionRuntimeState so neither struct carries a field that only
// means something for the other lifecycle.
USTRUCT(BlueprintType)
struct UNREALTURNBASEDMECHANICS_API FPermanentActionRuntimeState
{
    GENERATED_BODY()

    // The CURRENT effective cap on uses per turn (0 = unlimited). Seeded once
    // from FPermanentActionConfig::MaxUsesPerTurn and NOT reset each turn, so
    // game logic can change it temporarily (e.g. 2 -> 1 for a few turns) and
    // both the server gate (CanUseAction) and the UI read this, never the
    // config. Not a duplicate of the config value: the config is the base.
    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    int32 MaxUsesThisTurn = 0;

    // Resets to 0 every turn start -- the one counter both MaxUsesPerTurn
    // and every turn-end leaf's RequiredUsesThisTurn compare against.
    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    int32 UsesThisTurn = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    int32 CooldownTurnsRemaining = 0;
};

// Designer-authored config for an action with a match-level use budget
// (SWAP-style). StartingMatchUses is seeded into runtime state once; 0 means
// the player doesn't start with the action and may be granted uses mid-match,
// capped by MaxHeldUses.
USTRUCT(BlueprintType)
struct UNREALTURNBASEDMECHANICS_API FNumberedActionConfig
{
    GENERATED_BODY()

    // See FPermanentActionConfig::ActionClass
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Config")
    TSubclassOf<UTurnBasedAction> ActionClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Config", meta = (ClampMin = 0))
    int32 StartingMatchUses = 0;

    // 0 = unlimited
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Config", meta = (ClampMin = 0))
    int32 MaxUsesPerTurn = 0;

    // Most uses a player can hold at once (grants clamp to this). 0 = unlimited
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Config", meta = (ClampMin = 0))
    int32 MaxHeldUses = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Config", meta = (ClampMin = 0))
    int32 CooldownTurns = 0;
};

// Per-player, per-action live state for a numbered action.
USTRUCT(BlueprintType)
struct UNREALTURNBASEDMECHANICS_API FNumberedActionRuntimeState
{
    GENERATED_BODY()

    // Seeded from FNumberedActionConfig::StartingMatchUses once at match
    // start; never resets to that value. Goes down on use, up on a grant
    // (clamped to MaxHeldUses).
    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    int32 UsesRemaining = 0;

    // The CURRENT effective cap on uses per turn (0 = unlimited) -- see
    // FPermanentActionRuntimeState::MaxUsesThisTurn.
    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    int32 MaxUsesThisTurn = 0;

    // Resets to 0 every turn start -- the one counter both MaxUsesPerTurn
    // and every turn-end leaf's RequiredUsesThisTurn compare against.
    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    int32 UsesThisTurn = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    int32 CooldownTurnsRemaining = 0;
};

// A runtime-state struct paired with the action class it belongs to. PlayerState
// replicates arrays of these (a TMap can't replicate), keyed by class exactly
// like the config entries they were seeded from.
USTRUCT(BlueprintType)
struct UNREALTURNBASEDMECHANICS_API FPermanentActionRuntimeEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    TSubclassOf<UTurnBasedAction> ActionClass;

    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    FPermanentActionRuntimeState State;
};

USTRUCT(BlueprintType)
struct UNREALTURNBASEDMECHANICS_API FNumberedActionRuntimeEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    TSubclassOf<UTurnBasedAction> ActionClass;

    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    FNumberedActionRuntimeState State;
};
