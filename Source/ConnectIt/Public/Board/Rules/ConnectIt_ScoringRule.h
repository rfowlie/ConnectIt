// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "GridMechanicsBaseStructs.h"
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


// What scores, and how the board changes as a result, after a piece has
// arrived at a tile.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItScoringRule
{
    GENERATED_BODY()

    virtual ~FConnectItScoringRule() = default;

    // Faction now has a piece on Position (placed, swapped in, shifted in,
    // captured...). Apply any scoring to Board -- its ScoreBoard, and whatever
    // else scoring does to tiles and pieces -- and return the points scored.
    // bArrivingPieceSurvives: whether that piece stays on Position if it
    // scores (FConnectItBoardOperation::ArrivingPiecesSurviveScoring).
    // OutEvents, if not null, gets one FConnectItBoardEvent_Scored appended
    // per thing that scored (for visuals); the AI's search passes null.
    virtual float ApplyScoring(
        FConnectItBoardState& Board,
        FGridPosition Position,
        int32 Faction,
        bool bArrivingPieceSurvives,
        FConnectItBoardChangeEvent* OutEvents) const
    {
        return 0.f;
    }
};
