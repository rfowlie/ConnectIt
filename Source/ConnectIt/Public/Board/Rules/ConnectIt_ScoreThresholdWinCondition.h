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
};
