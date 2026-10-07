// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/Rules/ConnectIt_WinCondition.h"
#include "ConnectIt_ScoreThresholdWinCondition.generated.h"


// The classic condition: the first faction to reach WinScoreThreshold points
// wins.
USTRUCT(BlueprintType, meta = (DisplayName = "Score Threshold"))
struct CONNECTIT_API FConnectItWinCondition_ScoreThreshold : public FConnectItWinCondition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Win Condition", meta = (ClampMin = 1.0))
    float WinScoreThreshold = 100.f;

    virtual int32 GetWinningFaction(const FConnectItBoardState& Board) const override
    {
        for (int32 Faction = 0; Faction < Board.ScoreBoard.Num(); Faction++)
        {
            if (Board.ScoreBoard[Faction] >= WinScoreThreshold)
            {
                return Faction;
            }
        }
        return INDEX_NONE;
    }

    virtual float GetProgress(const FConnectItBoardState& Board, int32 Faction) const override
    {
        return WinScoreThreshold > 0.f
            ? FMath::Clamp(Board.GetScore(Faction) / WinScoreThreshold, 0.f, 1.f)
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
