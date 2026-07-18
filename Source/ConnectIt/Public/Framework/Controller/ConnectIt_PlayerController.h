// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "Action/TurnBasedActionsComponent.h"
#include "TurnBasedMechanicsStructs.h"
#include "ConnectIt_PlayerController.generated.h"

class AConnectIt_BoardManager;
class UConnectIt_BoardManagerComponent;
class AConnectIt_GameState;


UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API AConnectIt_PlayerController : public APlayerController
{
    GENERATED_BODY()

public:

    AConnectIt_PlayerController();

    // --- Components ---

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Components")
    TObjectPtr<UTurnBasedParticipantComponent> ParticipantComponent = nullptr;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Components")
    TObjectPtr<UTurnBasedActionsComponent> ActionsComponent = nullptr;

protected:

    virtual void BeginPlay() override;

private:

    // --- Initialisation ---

    // Finds board manager, initialises loadout, wires all delegates
    void InitialiseFromBoardManager();

    // --- Participant Delegate Handlers ---

    // Fired by participant component when it is our turn
    UFUNCTION()
    void HandleTurnNotificationReceived(const FTurnNotification& Notification);

    // Fired by participant component when it is opponent's turn
    UFUNCTION()
    void HandleOpponentTurnStarted(int32 ActiveParticipantSlotIndex);

    // --- Match Phase Handler ---

    // Fired by game state when match phase changes
    UFUNCTION()
    void HandleMatchPhaseChanged(EMatchPhase NewPhase);

    // --- Action Component Handlers ---

    // Routes board change request to server via ServerRPC
    UFUNCTION()
    void HandleBoardChangeRequested(const FTurnActionRequest& Request);

    // Routes turn end request to participant component ServerRPC
    UFUNCTION()
    void HandleTurnEndRequested();

    // --- ServerRPC ---

    // Routes FTurnActionRequest to server for validation
    // Board manager processes on server side
    UFUNCTION(Server, Reliable)
    void ServerRouteBoardChangeRequest(FTurnActionRequest Request);
    void ServerRouteBoardChangeRequest_Implementation(
        FTurnActionRequest Request);

    // Cached reference -- found once in InitialiseFromBoardManager
    UPROPERTY()
    TObjectPtr<AConnectIt_BoardManager> CachedBoardManager = nullptr;
    
    bool bIsPaused = false;
};