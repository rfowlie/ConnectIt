// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/Rules/ConnectIt_MatchCondition.h"
#include "ConnectIt_PieceCountCondition.generated.h"


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

    virtual bool HasFactionWon(const FConnectItBoardState& Board, int32 Faction) const override
    {
        if (!Board.ScoreBoard.IsValidIndex(Faction)) return false;
        return IsMet(CountPieces(Board, Faction));
    }

    virtual int32 GetWinningFaction(const FConnectItBoardState& Board) const override
    {
        for (int32 Faction = 0; Faction < Board.ScoreBoard.Num(); Faction++)
        {
            if (HasFactionWon(Board, Faction))
            {
                return Faction;
            }
        }
        return INDEX_NONE;
    }

    // How far the faction's count has travelled from where it started (the
    // board remembers each faction's starting count, from the designer's tile
    // data) to the nearest count that meets the condition. 0 if the board has
    // no starting count to measure from.
    virtual float GetProgress(const FConnectItBoardState& Board, int32 Faction) const override
    {
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
