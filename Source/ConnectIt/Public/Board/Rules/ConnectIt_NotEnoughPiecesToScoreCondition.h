// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/Rules/ConnectIt_MatchCondition.h"
#include "Board/Rules/ConnectIt_PieceCountCondition.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "ConnectIt_NotEnoughPiecesToScoreCondition.generated.h"


// Met when a faction has fewer pieces on the board than the match's scoring
// rule needs for a score (Lines: its Connect Length) -- it can never score
// again. No pieces at all counts too; a "clear the board" goal is checked
// before it, so reaching 0 by scoring is still that win.
//
// Meant for levels where a side can't add pieces (e.g. a swap-only exhaustion
// level): in a level where pieces are placed, having fewer than a line early
// on is normal. Never met when the scoring rule doesn't say how many pieces a
// score needs.
USTRUCT(BlueprintType, meta = (DisplayName = "Not Enough Pieces To Score"))
struct CONNECTIT_API FConnectItMatchCondition_NotEnoughPiecesToScore : public FConnectItMatchCondition
{
    GENERATED_BODY()

    virtual bool IsMetFor(const FConnectItConditionContext& Context, int32 Faction) const override
    {
        const int32 Minimum = GetMinimum(Context.Rules);
        if (Minimum <= 0 || !Context.Board.ScoreBoard.IsValidIndex(Faction)) return false;
        return FConnectItMatchCondition_PieceCount::CountPieces(Context.Board, Faction) < Minimum;
    }

    virtual void GetValidationWarnings(const FConnectItRuleSet* Rules, TArray<FText>& OutWarnings) const override
    {
        if (Rules && GetMinimum(*Rules) <= 0)
        {
            OutWarnings.Add(FText::FromString(TEXT(
                "Not Enough Pieces To Score: the match's scoring rule doesn't say how many pieces a score needs, "
                "so this condition never holds.")));
        }
    }

private:

    static int32 GetMinimum(const FConnectItRuleSet& Rules)
    {
        const FConnectItScoringRule* Scoring = Rules.GetScoringRule();
        return Scoring ? Scoring->GetMinPiecesToScore() : 0;
    }
};
