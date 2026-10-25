// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Board/Rules/Conditions/ConnectIt_MatchCondition.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Board/Operations/ConnectIt_BoardOperation.h"
#include "ConnectIt_MatchConditions.generated.h"


// The match conditions ConnectIt ships (FConnectItMatchCondition subtypes),
// one family in one file: a condition asset (UConnectIt_MatchConditionAsset)
// lists any of them. A new kind of condition goes here, next to the others.

// ---------------------------------------------------------------------------
// Score Threshold
// ---------------------------------------------------------------------------

// The classic condition: a faction has reached WinScoreThreshold points.
USTRUCT(BlueprintType, meta = (DisplayName = "Score Threshold"))
struct CONNECTIT_API FConnectItMatchCondition_ScoreThreshold : public FConnectItMatchCondition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Win Condition", meta = (ClampMin = 1.0))
    float WinScoreThreshold = 100.f;

    virtual bool IsMetFor(const FConnectItConditionContext& Context, int32 Faction) const override
    {
        return Context.Board.ScoreBoard.IsValidIndex(Faction) && Context.Board.ScoreBoard[Faction] >= WinScoreThreshold;
    }

    virtual float GetProgress(const FConnectItConditionContext& Context, int32 Faction) const override
    {
        return WinScoreThreshold > 0.f
            ? FMath::Clamp(Context.Board.GetScore(Faction) / WinScoreThreshold, 0.f, 1.f)
            : 0.f;
    }

    virtual bool GetTargetScore(float& OutTargetScore) const override
    {
        OutTargetScore = WinScoreThreshold;
        return true;
    }

    virtual bool SetTargetScore(float NewTargetScore) override
    {
        WinScoreThreshold = NewTargetScore;
        return true;
    }
};

// ---------------------------------------------------------------------------
// Piece Count
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EConnectItPieceCountComparison : uint8
{
    LessThan,
    Equal,
    GreaterThan,
};

// A faction meets this when the number of its pieces on the board compares to
// Count the chosen way. Equal 0 is the "clear your whole army" objective: pieces
// leave the board by scoring (a completed line clears), so it suits a level
// where pieces can't be placed (e.g. a swap-only puzzle).
//
// It is true for ANY faction with an empty board, including one that has not
// scored. Put a Score Threshold in the same group when a level should require
// scoring first, and give the AI its own conditions rather than letting it
// share an "Equal 0".
USTRUCT(BlueprintType, meta = (DisplayName = "Piece Count"))
struct CONNECTIT_API FConnectItMatchCondition_PieceCount : public FConnectItMatchCondition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Win Condition")
    EConnectItPieceCountComparison Comparison = EConnectItPieceCountComparison::Equal;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Win Condition", meta = (ClampMin = 0))
    int32 Count = 0;

    static int32 CountPieces(const FConnectItBoardState& Board, int32 Faction)
    {
        int32 Total = 0;
        for (const FConnectItTileData& Tile : Board.TileDataArray)
        {
            if (Tile.FactionPiece == Faction) Total++;
        }
        return Total;
    }

    virtual bool IsMetFor(const FConnectItConditionContext& Context, int32 Faction) const override
    {
        if (!Context.Board.ScoreBoard.IsValidIndex(Faction)) return false;
        return IsMet(CountPieces(Context.Board, Faction));
    }

    // How far the faction's count has travelled from where it started (the
    // board remembers each faction's starting count, from the designer's tile
    // data) to the nearest count that meets the condition. 0 if the board has
    // no starting count to measure from.
    virtual float GetProgress(const FConnectItConditionContext& Context, int32 Faction) const override
    {
        const FConnectItBoardState& Board = Context.Board;
        const int32 Current = CountPieces(Board, Faction);
        if (IsMet(Current)) return 1.f;
        if (!Board.StartingPieces.IsValidIndex(Faction)) return 0.f;

        const int32 Start = Board.StartingPieces[Faction];
        const int32 Goal = NearestMetCount();
        const int32 Distance = FMath::Abs(Start - Goal);
        if (Distance == 0) return 0.f;

        return FMath::Clamp(1.f - static_cast<float>(FMath::Abs(Current - Goal)) / Distance, 0.f, 1.f);
    }

private:

    bool IsMet(int32 Current) const
    {
        switch (Comparison)
        {
        case EConnectItPieceCountComparison::LessThan:    return Current < Count;
        case EConnectItPieceCountComparison::GreaterThan: return Current > Count;
        default:                                          return Current == Count;
        }
    }

    // The closest piece count that satisfies the comparison, from below for
    // GreaterThan and from above for LessThan
    int32 NearestMetCount() const
    {
        switch (Comparison)
        {
        case EConnectItPieceCountComparison::LessThan:    return Count - 1;
        case EConnectItPieceCountComparison::GreaterThan: return Count + 1;
        default:                                          return Count;
        }
    }
};

// ---------------------------------------------------------------------------
// Not Enough Pieces To Score
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// No Legal Move
// ---------------------------------------------------------------------------

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

    // The kind of move (its operation tag, e.g. ConnectIt.BoardOperation.SwapPieces)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition", meta = (Categories = "ConnectIt.BoardOperation"))
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
        else if (!FConnectItBoardOperation::GetAllOperationTags().Contains(Move))
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
