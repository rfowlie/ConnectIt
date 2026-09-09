// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Controller/TurnBasedPlayerControllerBase.h"
#include "TurnBasedMechanicsStructs.h"
#include "ConnectIt_PlayerController.generated.h"

// ConnectIt player controller
// Generic turn/action/match-phase wiring (ParticipantComponent,
// ActionsComponent, delegate routing) is provided by the base class via
// UTurnBasedControllerCoordinatorComponent -- this class only adds the
// ConnectIt-specific plumbing: loading the player's action loadout from the
// level config, and routing board change requests to the server for
// validation. Tile/piece registries used to live here (per-machine, one
// controller per machine) but have moved to UConnectIt_BoardRegistrySubsystem
// -- they're level-authored, deterministic, world-scoped singletons, not
// per-machine or per-player state. See that class's header comment.
UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API AConnectIt_PlayerController : public ATurnBasedPlayerControllerBase
{
    GENERATED_BODY()

protected:

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

    // --- Initialisation ---

    // Resolves the level config asset and initialises this player's loadout
    void InitialiseFromLevelConfig();

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
};
