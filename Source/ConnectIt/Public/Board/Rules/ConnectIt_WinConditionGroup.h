// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "Board/Rules/ConnectIt_WinCondition.h"
#include "ConnectIt_WinConditionGroup.generated.h"


// One goal, made of one or more win conditions that must ALL hold at the same
// time (e.g. Piece Count Equal 0 AND Score Threshold 4: clear the board, having
// scored). A group with no conditions is never satisfied -- "nothing to
// satisfy" is not a win. (All-of is the only way conditions combine today; any
// other way would be a setting on the group, not a change to how groups are
// listed.)
//
// A rule set lists a side's groups in the order they must be completed (see
// FConnectItRuleSet::PlayerWinGroups): each completed group is a checkpoint,
// and completing the last one is the win.
//
// A plain thread-safe struct like the conditions it holds (see the note at the
// top of ConnectIt_ScoringRule.h).
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItWinConditionGroup
{
    GENERATED_BODY()

    // Optional label for this goal ("Score 20", "Clear the board"): carried by
    // the checkpoint event and delegate and shown in logs, so a level
    // Blueprint can tell checkpoints apart without counting.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Win Conditions")
    FName Name;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Win Conditions", meta = (ExcludeBaseStruct))
    TArray<TInstancedStruct<FConnectItWinCondition>> Conditions;

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
