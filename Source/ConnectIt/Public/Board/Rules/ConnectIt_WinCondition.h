// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ConnectIt_Structs.h"
#include "ConnectIt_WinCondition.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable, BlueprintType)
class UConnectIt_WinCondition : public UInterface
{
    GENERATED_BODY()
};

// Pluggable win-condition strategy -- whether the game has been won after
// the current move's scoring has been applied. Assigned via
// UConnectIt_BoardRulesComponent::WinConditionRule (TScriptInterface,
// EditAnywhere Instanced), mirroring ITurnOrderInterface/TurnOrderStrategy's
// precedent. UConnectIt_ScoreThresholdWinCondition is the default
// implementation; other conditions (e.g. occupying a set of tiles instead
// of a score threshold) can be added as new implementers without touching
// AConnectIt_BoardManager.
class CONNECTIT_API IConnectIt_WinCondition
{
    GENERATED_BODY()

public:

    // Called after scoring has been applied for the current move.
    // Implementations set MutableState.bGameOver / WinningFactionSlot when
    // their condition is met.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt|WinCondition")
    void CheckWinCondition(UPARAM(ref) FConnectItBoardState& MutableState);
};
