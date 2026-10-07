// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/Rules/ConnectIt_ScoringRule.h"
#include "ConnectIt_LineScoringRule.generated.h"


// The classic rule: a straight line of ConnectLength or more of one faction's
// pieces through the arriving piece scores. Points = the sum of the line's
// tile multipliers; every tile in the line then loses its piece and gains +1
// multiplier, except the arriving piece, which stays. Each completed line is
// one scoring configuration.
USTRUCT(BlueprintType, meta = (DisplayName = "Lines"))
struct CONNECTIT_API FConnectItScoringRule_Lines : public FConnectItScoringRule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring", meta = (ClampMin = 3))
    int32 ConnectLength = 4;

    virtual float ApplyScoring(
        FConnectItBoardState& Board,
        FGridPosition Position,
        int32 Faction,
        TArray<FConnectItScoringConfiguration>* OutConfigurations) const override;

    // The four line axes scoring checks (each walked both ways). Public so
    // the MinMax line-potential evaluation measures along exactly the same
    // axes.
    static const TArray<FGridDirectionVector>& GetScoringDirections();

private:

    static TArray<TArray<FGridPosition>> FindScoringLines(
        const FConnectItBoardState& State,
        FGridPosition Position,
        int32 FactionSlot,
        int32 ConnectLength);

    static float ApplyScoringLine(
        FConnectItBoardState& MutableState,
        const TArray<FGridPosition>& Line,
        FGridPosition CompletingPosition,
        int32 FactionSlot);
};
