// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Controller/TurnBasedPlayerControllerBase.h"
#include "TurnBasedMechanicsStructs.h"
#include "ConnectIt_PlayerController.generated.h"

class AConnectIt_BoardManager;
class UConnectIt_BoardManagerComponent;

// ConnectIt player controller
// Generic turn/action/match-phase wiring (ParticipantComponent,
// ActionsComponent, delegate routing) is provided by the base class via
// UTurnBasedControllerCoordinatorComponent -- this class only adds the
// ConnectIt-specific plumbing: finding the board manager, loading the
// player's action loadout, and routing board change requests to the
// server for validation
UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API AConnectIt_PlayerController : public ATurnBasedPlayerControllerBase
{
    GENERATED_BODY()

public:

    
protected:

    virtual void BeginPlay() override;

private:

    // --- Initialisation ---

    // Finds board manager, initialises loadout
    void InitialiseFromBoardManager();

    // --- Action Component Handler ---
    // Board change request routing is ConnectIt-specific -- the generic
    // coordinator only wires turn/action/match-phase plumbing

    // Routes board change request to server via ServerRPC
    UFUNCTION()
    void HandleBoardChangeRequested(const FTurnActionRequest& Request);

    // --- ServerRPC ---

    // Routes FTurnActionRequest to server for validation
    // Board manager processes on server side
    UFUNCTION(Server, Reliable)
    void ServerRouteBoardChangeRequest(FTurnActionRequest Request);
    void ServerRouteBoardChangeRequest_Implementation(
        FTurnActionRequest Request);

    // --- ClientRPC ---

    // Reports the server's accept/reject answer for Request back to this
    // client -- called from every exit path of
    // ServerRouteBoardChangeRequest_Implementation, not just the ones that
    // reach ProcessRequest. Forwards straight into
    // UTurnBasedActionsComponent::NotifyBoardChangeOutcome, the generic
    // entry point that actually resolves the awaiting-confirmation state.
    UFUNCTION(Client, Reliable)
    void ClientNotifyBoardChangeOutcome(FTurnActionRequest Request, bool bSucceeded);

    // Cached reference -- found once in InitialiseFromBoardManager
    UPROPERTY()
    TObjectPtr<AConnectIt_BoardManager> CachedBoardManager = nullptr;
};
