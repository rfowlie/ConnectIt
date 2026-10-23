// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "GridMechanicsBaseStructs.h"
#include "Board/ConnectIt_BoardChanges.h"
#include "ConnectIt_ScoringRule.generated.h"


// --- How the board's rules are built (applies to every rule struct) ---------
// Rules are plain polymorphic structs, held in a FConnectItRuleSet and picked /
// configured per level in the Details panel. They are deliberately NOT
// UObjects or Blueprint interfaces: the real game (the server's Mediator) and
// the AI's background search both call the very same rule, so a rule must be
// safe to call from any thread --
//   * a pure function of its inputs (the board it is handed + its own
//     properties),
//   * no UObject references, no mutable state.
// New kinds of rule are new C++ structs deriving from the bases.
// ----------------------------------------------------------------------------


// What scores, and how the board changes as a result, after pieces have
// arrived on tiles.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItScoringRule
{
    GENERATED_BODY()

    virtual ~FConnectItScoringRule() = default;

    // One scoring pass: Arrivals are the pieces that may now score (placed,
    // swapped in, shifted, captured...), each with whether it stays on its
    // tile if it scores. Everything that scores is found on Board as it is,
    // then all of it is applied at once -- so two arrivals can't break each
    // other's scores. Apply it to Board (its ScoreBoard, and whatever else
    // scoring does to tiles and pieces) and return the points scored.
    // OutChanges gets what scoring did: the positions it ran through
    // (ScoredThrough) and the tiles whose multiplier changed (TileChanged).
    // OutEvents, if not null, gets one FConnectItBoardEvent_Scored appended
    // per thing that scored (for visuals); the AI's search passes null.
    virtual float ScorePass(
        FConnectItBoardState& Board,
        TConstArrayView<FConnectItPieceArrival> Arrivals,
        FConnectItBoardChanges& OutChanges,
        FConnectItBoardChangeEvent* OutEvents) const
    {
        return 0.f;
    }

    // The fewest pieces of one faction a score can be made from (Lines: its
    // Connect Length). 0 = this rule doesn't say. Read by conditions such as
    // Not Enough Pieces To Score.
    virtual int32 GetMinPiecesToScore() const
    {
        return 0;
    }
};
