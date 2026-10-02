// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinMax/ConnectIt_MinMaxTerms.h"
#include "ConnectIt_MinMaxTestTerms.generated.h"


// Test-only evaluation term (ConnectIt.AI.MinMax.EvaluationTermsDriveChoice):
// whoever owns the (0,0) corner is winning. Not meant for real strategies.
USTRUCT(meta = (Hidden, DisplayName = "(Test) Corner Owner"))
struct FConnectItMinMaxEvalTerm_TestCorner : public FConnectItMinMaxEvalTerm
{
    GENERATED_BODY()

    virtual float Evaluate(
        const FConnectItMinMaxRules& Rules, const FConnectItBoardState& Board, int32 Side) const override
    {
        const FConnectItTileData* Corner = Board.GetTileData(FGridPosition(0, 0));
        if (!Corner || Corner->FactionPiece == -1) return 0.f;
        return Corner->FactionPiece == Side ? 1.f : -1.f;
    }
};
