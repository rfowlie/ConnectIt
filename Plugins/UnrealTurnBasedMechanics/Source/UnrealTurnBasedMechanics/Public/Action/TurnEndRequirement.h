// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "TurnEndRequirement.generated.h"

class UTurnBasedAction;

// A loadout's turn-end requirement is a tree of these nodes. Nodes are
// Instanced UObjects rather than a recursive struct because UnrealHeaderTool
// rejects a USTRUCT holding an array of itself; the same pattern the
// loadout's Actions array already uses.
//
// The uses threshold lives on the leaf, not the action, on purpose: the same
// action can appear in several leaves that each need a different count
// (e.g. "Place x2" OR "Place x1 AND Swap x1"), all reading the one
// underlying uses-this-turn counter.

// How a group combines its enabled children.
UENUM(BlueprintType)
enum class ETurnEndCombineMode : uint8
{
    // Satisfied once ANY enabled child is satisfied
    Any,
    // Satisfied only when EVERY enabled child is satisfied
    All
};

// Skipped nodes take no part in their parent's Any/All -- distinct from
// Unsatisfied, otherwise disabling one child of an All group would brick the
// whole group instead of loosening it.
enum class ETurnEndRequirementResult : uint8
{
    Skipped,
    Satisfied,
    Unsatisfied
};

// Maps an action tag to how many times that action has completed this turn
// (0 if unknown).
using FTurnEndUsesLookup = TFunctionRef<int32(const FGameplayTag&)>;

UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class UNREALTURNBASEDMECHANICS_API UTurnEndRequirementNode : public UObject
{
    GENERATED_BODY()

public:

    // Disabled = skipped, exactly as if absent from its parent's list -- an
    // enabled sibling can still satisfy the group without it. Designer aid
    // for switching a leaf/subtree off while testing without deleting it.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turn End")
    bool bIsEnabled = true;

    // Skipped if disabled or if nothing under it is enabled
    virtual ETurnEndRequirementResult Evaluate(FTurnEndUsesLookup GetUsesThisTurn) const
    {
        return ETurnEndRequirementResult::Skipped;
    }

    // Root-level convenience: true only if the tree resolves to Satisfied.
    // A root that resolves to Skipped (everything disabled/empty) is not
    // satisfied.
    bool IsSatisfied(FTurnEndUsesLookup GetUsesThisTurn) const
    {
        return Evaluate(GetUsesThisTurn) == ETurnEndRequirementResult::Satisfied;
    }
};

// Satisfied once its action has completed RequiredUsesThisTurn times this turn
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced,
    meta = (DisplayName = "Action Used"))
class UNREALTURNBASEDMECHANICS_API UTurnEndRequirementLeaf : public UTurnEndRequirementNode
{
    GENERATED_BODY()

public:

    // The action this leaf counts; its tag is derived from the class
    // (UTurnBasedAction::GetTagForClass), never typed here.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turn End")
    TSubclassOf<UTurnBasedAction> ActionClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turn End", meta = (ClampMin = 1))
    int32 RequiredUsesThisTurn = 1;

    virtual ETurnEndRequirementResult Evaluate(
        FTurnEndUsesLookup GetUsesThisTurn) const override;
};

// Combines its enabled children with Any/All
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced,
    meta = (DisplayName = "Group"))
class UNREALTURNBASEDMECHANICS_API UTurnEndRequirementGroup : public UTurnEndRequirementNode
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turn End")
    ETurnEndCombineMode Mode = ETurnEndCombineMode::Any;

    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Turn End")
    TArray<TObjectPtr<UTurnEndRequirementNode>> Children;

    virtual ETurnEndRequirementResult Evaluate(
        FTurnEndUsesLookup GetUsesThisTurn) const override;
};
