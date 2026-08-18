// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/Rules/ConnectIt_ScoringRule.h"
#include "UObject/Object.h"
#include "ConnectIt_LineScoringRule.generated.h"

// Default IConnectIt_ScoringRule implementation -- scores an N-in-a-row line
// (ConnectLength) through the just-placed piece in any of the four grid
// directions. Ported unchanged from AConnectIt_BoardManager's previous
// hardcoded CheckAndApplyScoring/FindScoringLines/ApplyScoringLine.
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CONNECTIT_API UConnectIt_LineScoringRule : public UObject, public IConnectIt_ScoringRule
{
    GENERATED_BODY()

public:

    // How many tiles in a line are required to score
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConnectIt|Scoring",
        meta = (ClampMin = 3))
    int32 ConnectLength = 4;

    virtual float ApplyScoring_Implementation(
        FConnectItBoardState& MutableState,
        FGridPosition Position,
        int32 FactionSlot,
        TArray<FGridPosition>& OutScoringPositions) override;

private:

    TArray<TArray<FGridPosition>> FindScoringLines(
        const FConnectItBoardState& State,
        FGridPosition Position,
        int32 FactionSlot) const;

    float ApplyScoringLine(
        FConnectItBoardState& MutableState,
        const TArray<FGridPosition>& Line,
        FGridPosition CompletingPosition,
        int32 FactionSlot) const;

    static const TArray<FGridDirectionVector>& GetScoringDirections();
};
