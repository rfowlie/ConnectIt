// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/Rules/ConnectIt_ScoringRule.h"
#include "ConnectIt_LineScoringRule.generated.h"


// Which ways a line may run to score under FConnectItScoringRule_Lines. Flags:
// any combination can be allowed at once.
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EConnectItScoringDirection : uint8
{
    None       = 0 UMETA(Hidden),

    // Along a row
    Horizontal = 1 << 0,

    // Along a column
    Vertical   = 1 << 1,

    // Along either diagonal
    Diagonal   = 1 << 2,
};
ENUM_CLASS_FLAGS(EConnectItScoringDirection);

// The classic rule: a straight line of ConnectLength or more of one faction's
// pieces through an arriving piece scores, if it runs in an allowed
// direction (ScoringDirections). Points = the sum of the line's
// tile multipliers; every tile in the line then loses its piece and gains +1
// multiplier, except the line's kept piece: the first arrival on it that
// survives scoring (none when the operation says its pieces don't survive).
// A kept piece is never cleared by another line through it. A pass finds
// every line from every arrival first (a line two arrivals share counts
// once), then applies them in order -- a tile in two lines scores in both and
// gains +1 for each. Each completed line is its own Scored event.
USTRUCT(BlueprintType, meta = (DisplayName = "Lines"))
struct CONNECTIT_API FConnectItScoringRule_Lines : public FConnectItScoringRule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring", meta = (ClampMin = 3))
    int32 ConnectLength = 4;

    // Which ways a line may run to score. Default: all of them. A line in a
    // direction that isn't ticked simply doesn't count (and the AI stops
    // valuing it). With nothing ticked, nothing can ever score.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring",
        meta = (Bitmask, BitmaskEnum = "/Script/ConnectIt.EConnectItScoringDirection"))
    int32 ScoringDirections = static_cast<int32>(
        EConnectItScoringDirection::Horizontal
        | EConnectItScoringDirection::Vertical
        | EConnectItScoringDirection::Diagonal);

    bool IsDirectionAllowed(EConnectItScoringDirection Direction) const
    {
        return (ScoringDirections & static_cast<int32>(Direction)) != 0;
    }

    virtual float ScorePass(
        FConnectItBoardState& Board,
        TConstArrayView<FConnectItPieceArrival> Arrivals,
        FConnectItBoardChanges& OutChanges,
        FConnectItBoardChangeEvent* OutEvents) const override;

    virtual int32 GetMinPiecesToScore() const override { return ConnectLength; }

    // The line axes scoring checks under ScoringDirections (each is walked
    // both ways): up to four -- the row axis, the column axis and the two
    // diagonals. Public so the MinMax line-potential evaluation measures
    // along exactly the same axes. Inline storage: called for every position
    // the AI's search tries.
    using FAxes = TArray<FGridDirectionVector, TInlineAllocator<4>>;
    FAxes GetAllowedAxes() const;

private:

    TArray<TArray<FGridPosition>> FindScoringLines(
        const FConnectItBoardState& State,
        FGridPosition Position,
        int32 FactionSlot) const;

    // Clears the line's pieces, raises its tiles' multipliers (within each
    // tile type's limits, from Changes' rules), returns the points. A piece
    // on a position in KeptPositions is left standing. OutClearedPositions,
    // if not null, gets every position a piece was actually removed from.
    static float ApplyScoringLine(
        FConnectItBoardState& MutableState,
        const TArray<FGridPosition>& Line,
        TConstArrayView<FGridPosition> KeptPositions,
        TArray<FGridPosition>* OutClearedPositions,
        const FConnectItBoardChanges& Changes);
};
