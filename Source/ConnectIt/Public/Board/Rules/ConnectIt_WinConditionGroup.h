// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "Board/Rules/ConnectIt_WinCondition.h"
#include "ConnectIt_WinConditionGroup.generated.h"

class UConnectIt_WinConditionAsset;

// One goal during a match: the copy of a UConnectIt_WinConditionAsset's
// conditions that the rule set plays with (FConnectItRuleSet::ResolveWinGroups
// makes it). One or more win conditions that must ALL hold at the same time. A
// group with no conditions is never satisfied -- "nothing to satisfy" is not a
// win. (All-of is the only way conditions combine today; any other way would
// be a setting on the asset, not a change to how goals are listed.)
//
// A rule set holds a side's groups in the order they must be completed (see
// FConnectItRuleSet::PlayerWinGroups): each completed group is a checkpoint,
// and completing the last one is the win.
//
// A plain thread-safe struct like the conditions it holds (see the note at the
// top of ConnectIt_ScoringRule.h). Source is only carried along, never
// dereferenced off the game thread.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItWinConditionGroup
{
    GENERATED_BODY()

    // The goal's label for logs and the checkpoint event: the source asset's
    // name
    UPROPERTY(BlueprintReadOnly, Category = "Win Conditions")
    FName Name;

    UPROPERTY(BlueprintReadOnly, Category = "Win Conditions")
    TArray<TInstancedStruct<FConnectItWinCondition>> Conditions;

    // The asset the conditions were copied from (null for a group built in code)
    UPROPERTY(BlueprintReadOnly, Category = "Win Conditions")
    TObjectPtr<UConnectIt_WinConditionAsset> Source = nullptr;

    bool IsEmpty() const { return Conditions.IsEmpty(); }

    // Non-empty, and every condition says Faction has met it.
    bool HasFactionWon(const FConnectItBoardState& Board, int32 Faction) const
    {
        if (Conditions.IsEmpty()) return false;

        for (const TInstancedStruct<FConnectItWinCondition>& Entry : Conditions)
        {
            const FConnectItWinCondition* Condition = Entry.GetPtr();
            // A cleared slot can't be satisfied, so it blocks the win rather
            // than being skipped (skipping would let a half-set group win).
            if (!Condition || !Condition->HasFactionWon(Board, Faction)) return false;
        }
        return true;
    }

    // The average of the conditions' progress, so it only reaches 1 when every
    // one does. 0 for an empty group.
    float GetProgress(const FConnectItBoardState& Board, int32 Faction) const
    {
        if (Conditions.IsEmpty()) return 0.f;

        float Total = 0.f;
        for (const TInstancedStruct<FConnectItWinCondition>& Entry : Conditions)
        {
            const FConnectItWinCondition* Condition = Entry.GetPtr();
            Total += Condition ? Condition->GetProgress(Board, Faction) : 0.f;
        }
        return Total / Conditions.Num();
    }

    // The first score-based condition's target. False if none is score-based.
    bool GetTargetScore(float& OutTargetScore) const
    {
        for (const TInstancedStruct<FConnectItWinCondition>& Entry : Conditions)
        {
            const FConnectItWinCondition* Condition = Entry.GetPtr();
            if (Condition && Condition->GetTargetScore(OutTargetScore)) return true;
        }
        return false;
    }

    // Sets the target of every score-based condition. False if none accepted.
    bool SetTargetScore(float NewTargetScore)
    {
        bool bAnySet = false;
        for (TInstancedStruct<FConnectItWinCondition>& Entry : Conditions)
        {
            if (FConnectItWinCondition* Condition = Entry.GetMutablePtr())
            {
                bAnySet |= Condition->SetTargetScore(NewTargetScore);
            }
        }
        return bAnySet;
    }
};
