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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConnectIt|Scoring", meta = (ClampMin = 3))
    int32 ConnectLength = 4;

    virtual float ApplyScoring_Implementation(
        FConnectItBoardState& MutableState,
        FGridPosition Position,
        int32 FactionSlot,
        TArray<FGridPosition>& OutScoringPositions) override;

    // Server-only -- 0 for a Blueprint override that doesn't implement this
    // (the default 0 return on IConnectIt_ScoringRule::GetMinimumConnectLength
    // means "unknown," not "no minimum"). Mirrors GetTargetScore's role on
    // the win-condition interface: lets non-search code (the Classic MinMax
    // AI) read this rule's config polymorphically, without knowing it's the
    // concrete C++ class.
    virtual int32 GetMinimumConnectLength_Implementation() const override
    {
        return ConnectLength;
    }

    // Same algorithm ApplyScoring_Implementation wraps, as a plain static
    // function -- no UObject, no BlueprintNativeEvent dispatch. For search
    // code (Classic MinMax) that must not touch this UObject or run Blueprint
    // VM code off the game thread; see FConnectItMinMaxRules' class
    // comment. Takes ConnectLength explicitly instead of reading the
    // instance member, so a caller can pass whichever level's config
    // resolved (read once, on the game thread, via GetMinimumConnectLength).
    static float ApplyLineScoring(
        FConnectItBoardState& MutableState,
        FGridPosition Position,
        int32 FactionSlot,
        int32 ConnectLength,
        TArray<FGridPosition>& OutScoringPositions);

    // The four line axes scoring checks (each walked both ways). Public so the
    // Classic MinMax evaluator measures line potential along exactly the same
    // axes the real rule scores on.
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
