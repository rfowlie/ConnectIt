// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "GridMechanicsBaseStructs.h"
#include "TurnBasedMechanicsStructs.h"
#include "ConnectItGameBoard.generated.h"

// Singlecast mirrors of ConnectIt's score/game-over delegates -- see
// GameBoard.h's own comment for why Bind functions (not exposed delegate
// properties) are the correct interface mechanism.
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnScoreUpdatedHandler, int32, FactionSlot, float, NewScore);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnGameOverDetectedHandler, int32, WinningFactionSlot);

// ConnectIt tier of the GameBoard contract -- adds rules/score/lifecycle
// on top of IGameBoard's grid-mechanics-generic tier. Lives here, not in
// UnrealGridMechanics, because it's typed against FConnectItBoardState/
// FTurnActionRequest -- types that plugin has no module dependency on and
// shouldn't, since referencing them would make the "generic" plugin
// ConnectIt-specific. UE interface inheritance (IChild : public IParent,
// both with their own UINTERFACE) is natively supported.
//
// Deliberately not implemented anywhere yet -- see the plan history for
// why (no real consumer today to validate the contract's shape against).
UINTERFACE(Blueprintable, BlueprintType)
class CONNECTIT_API UConnectItGameBoard : public UInterface
{
    GENERATED_BODY()
};

class CONNECTIT_API IConnectItGameBoard
{
    GENERATED_BODY()

public:

    // --- State ---

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|State")
    FConnectItBoardState State_GetCurrentState() const;

    // --- Rules ---

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Rules")
    float Rules_ApplyScoring(
        UPARAM(ref) FConnectItBoardState& MutableState,
        FGridPosition Position,
        int32 FactionSlot,
        UPARAM(ref) TArray<FGridPosition>& OutScoringPositions);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Rules")
    void Rules_CheckWinCondition(UPARAM(ref) FConnectItBoardState& MutableState);

    // --- Score ---

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Score")
    float Score_GetFactionScore(int32 FactionSlot) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Score")
    void Score_BindOnScoreUpdated(const FOnScoreUpdatedHandler& Handler);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Score")
    void Score_BindOnGameOverDetected(const FOnGameOverDetectedHandler& Handler);

    // --- Lifecycle ---

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Lifecycle")
    void Lifecycle_InitialiseBoard(int32 NumFactions);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Lifecycle")
    bool Lifecycle_ProcessRequest(const FTurnActionRequest& Request);
};
