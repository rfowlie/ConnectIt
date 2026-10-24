// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionConfig.generated.h"

class UTurnBasedPlayerAction;

// One permanent player action a participant has in a match, and how often it
// may be used. Permanent uses come back every match; they aren't spent from
// anything the player keeps (item uses are). The numbers belong to the set-up
// -- a loadout, or a list a game builds (e.g. from a save) -- never to the
// action itself.
USTRUCT(BlueprintType)
struct UNREALTURNBASEDMECHANICS_API FPermanentActionConfig
{
    GENERATED_BODY()

    // The action this config applies to. Its tag is derived from the class
    // (UTurnBasedPlayerAction::GetTagForClass), never typed here.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Config")
    TSubclassOf<UTurnBasedPlayerAction> ActionClass;

    // Uses in the whole match. -1 = unlimited.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Config", meta = (ClampMin = -1))
    int32 MatchUses = -1;

    // Uses per turn. Always a limit, at least 1: unlimited uses in a turn
    // would let a turn go on forever.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Config", meta = (ClampMin = 1))
    int32 TurnUses = 1;

    // Turns before it can be used again after a use. 0 = none.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Config", meta = (ClampMin = 0))
    int32 CooldownTurns = 0;

    bool HasUnlimitedMatchUses() const { return MatchUses < 0; }
};

// Per-player, per-action live state for a permanent action, seeded from its
// FPermanentActionConfig.
USTRUCT(BlueprintType)
struct UNREALTURNBASEDMECHANICS_API FPermanentActionRuntimeState
{
    GENERATED_BODY()

    // Uses left in the match. -1 = unlimited; goes down on each use
    // otherwise.
    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    int32 MatchUsesRemaining = -1;

    // The CURRENT effective cap on uses per turn (at least 1). Seeded once
    // from FPermanentActionConfig::TurnUses and NOT reset each turn, so game
    // logic can change it temporarily (e.g. 2 -> 1 for a few turns) and both
    // the server gate (CanUseAction) and the UI read this, never the config.
    // Not a duplicate of the config value: the config is the base.
    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    int32 MaxUsesThisTurn = 1;

    // Resets to 0 every turn start -- the one counter both MaxUsesThisTurn
    // and every turn-end leaf's RequiredUsesThisTurn compare against.
    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    int32 UsesThisTurn = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    int32 CooldownTurnsRemaining = 0;

    // Turns of cooldown a use starts (the config's, kept here so the state
    // carries everything a use needs)
    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    int32 CooldownTurns = 0;
};

// A runtime-state struct paired with the action class it belongs to. PlayerState
// replicates an array of these (a TMap can't replicate), keyed by class exactly
// like the config entries they were seeded from.
USTRUCT(BlueprintType)
struct UNREALTURNBASEDMECHANICS_API FPermanentActionRuntimeEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    TSubclassOf<UTurnBasedPlayerAction> ActionClass;

    UPROPERTY(BlueprintReadOnly, Category = "Action State")
    FPermanentActionRuntimeState State;
};
