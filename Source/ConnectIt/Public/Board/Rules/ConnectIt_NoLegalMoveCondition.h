// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Board/Rules/ConnectIt_MatchCondition.h"
#include "Board/Operations/ConnectIt_BoardOperation.h"
#include "ConnectIt_NoLegalMoveCondition.generated.h"


// Met when a faction has no legal move of one kind on the board -- e.g. "No
// Legal Move: Swap Pieces" when there is no pair of its own and another
// piece to trade. Looks at the board only, not at the faction's loadout or
// uses left. For "can't place and can't swap", put two of these in one asset.
//
// Each kind of move answers the question itself
// (FConnectItBoardOperation::HasAnyLegalInstance); a kind that can't answer
// yet makes this condition never hold, and validation says so.
USTRUCT(BlueprintType, meta = (DisplayName = "No Legal Move"))
struct CONNECTIT_API FConnectItMatchCondition_NoLegalMove : public FConnectItMatchCondition
{
    GENERATED_BODY()

    // The kind of move (its request type, e.g. ConnectIt.Game.State.SwapPieces)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition", meta = (Categories = "ConnectIt.Game.State"))
    FGameplayTag Move;

    virtual bool IsMetFor(const FConnectItConditionContext& Context, int32 Faction) const override
    {
        return Context.Board.ScoreBoard.IsValidIndex(Faction)
            && FConnectItBoardOperation::GetLegality(Move, Context.Board, Faction) == EConnectItMoveLegality::NoLegalMove;
    }

    virtual void GetValidationWarnings(const FConnectItRuleSet* Rules, TArray<FText>& OutWarnings) const override
    {
        if (!Move.IsValid())
        {
            OutWarnings.Add(FText::FromString(TEXT("No Legal Move: no Move set, so this condition never holds.")));
        }
        else if (!FConnectItBoardOperation::GetAllRequestTypes().Contains(Move))
        {
            OutWarnings.Add(FText::FromString(FString::Printf(TEXT(
                "No Legal Move: '%s' is not a kind of board move, so this condition never holds."),
                *Move.ToString())));
        }
        // Unknown on an empty board means the kind of move can't answer at all
        else if (FConnectItBoardOperation::GetLegality(Move, FConnectItBoardState(), 0) == EConnectItMoveLegality::Unknown)
        {
            OutWarnings.Add(FText::FromString(FString::Printf(TEXT(
                "No Legal Move: '%s' can't tell yet whether it has a legal move, so this condition never holds."),
                *Move.ToString())));
        }
    }
};
