// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/Rules/ConnectIt_WinCondition.h"
#include "UObject/Object.h"
#include "ConnectIt_ScoreThresholdWinCondition.generated.h"

// Default IConnectIt_WinCondition implementation -- first faction to reach
// WinScoreThreshold points wins. Ported unchanged from AConnectIt_BoardManager's
// previous hardcoded CheckWinCondition.
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CONNECTIT_API UConnectIt_ScoreThresholdWinCondition : public UObject, public IConnectIt_WinCondition
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConnectIt|WinCondition")
    float WinScoreThreshold = 100.f;

    virtual void CheckWinCondition_Implementation(
        FConnectItBoardState& MutableState) override;

    virtual float GetTargetScore_Implementation() const override
    {
        return WinScoreThreshold;
    }

    virtual bool SetTargetScore_Implementation(float NewTargetScore) override
    {
        WinScoreThreshold = NewTargetScore;
        return true;
    }

    virtual TSharedPtr<const FConnectItWinCheck, ESPMode::ThreadSafe> MakeSearchWinCheck() const override;

    // The rule itself: the first faction slot at or above Threshold, or
    // INDEX_NONE. Shared by CheckWinCondition and the AI's win check so the
    // two can't drift.
    static int32 GetWinningFaction(const TArray<float>& ScoreBoard, float Threshold);
};

// UConnectIt_ScoreThresholdWinCondition's test with its threshold baked in --
// see FConnectItWinCheck.
struct CONNECTIT_API FConnectItScoreThresholdWinCheck final : public FConnectItWinCheck
{
    explicit FConnectItScoreThresholdWinCheck(float InThreshold) : Threshold(InThreshold) {}

    virtual int32 GetWinningFaction(const FConnectItBoardState& Board) const override
    {
        return UConnectIt_ScoreThresholdWinCondition::GetWinningFaction(Board.ScoreBoard, Threshold);
    }

    float Threshold = 0.f;
};
