// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Controller/TurnBasedPlayerControllerBase.h"
#include "TurnBasedMechanicsStructs.h"
#include "ConnectIt_PlayerController.generated.h"

class UActionLoadoutDataAsset;

// ConnectIt player controller
// Generic turn/action/match-phase wiring (ParticipantComponent,
// ActionsComponent, delegate routing) is provided by the base class via
// UTurnBasedControllerCoordinatorComponent -- this class only adds the
// ConnectIt-specific plumbing: building the player's actions from the loadout
// on their PlayerState (seeded by the server -- never from static level
// data, so it stays right if the server changes it), and routing board
// change requests to the server for validation. Tile/piece registries used to live here (per-machine, one
// controller per machine) but have moved to UConnectIt_BoardRegistrySubsystem
// -- they're level-authored, deterministic, world-scoped singletons, not
// per-machine or per-player state. See that class's header comment.
UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API AConnectIt_PlayerController : public ATurnBasedPlayerControllerBase
{
    GENERATED_BODY()

public:

    // Installs UConnectIt_TurnBasedActionsComponent in place of the base's
    // hardcoded UTurnBasedActionsComponent (see base ctor's
    // CreateDefaultSubobject<UTurnBasedActionsComponent>(TEXT("ActionsComponent")))
    // -- the standard FObjectInitializer::SetDefaultSubobjectClass override
    // technique, so PlacePiece/SWAP's alternate-required turn-end logic
    // lives here, not in a game-specific fork of the plugin's component.
    // Requires ATurnBasedPlayerControllerBase's own constructor to accept
    // and forward FObjectInitializer (added there for exactly this) -- a
    // bare zero-arg Super() has no argument slot to carry the override
    // through, this doesn't work with a plain Super().
    explicit AConnectIt_PlayerController(const FObjectInitializer& ObjectInitializer);

protected:

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Clients: the PlayerState has arrived -- its loadout may be on it
    virtual void OnRep_PlayerState() override;

private:

    // --- Initialisation ---

    // Local controller only. Binds to the PlayerState's OnLoadoutChanged and
    // builds the actions component from the PlayerState's current loadout, if
    // it has one and it isn't the one already in use. Called from BeginPlay,
    // OnRep_PlayerState and OnLoadoutChanged, because on a client the
    // PlayerState and its loadout can each arrive after this controller
    // starts; safe to call repeatedly.
    void InitialiseActionsFromPlayerState();

    UFUNCTION()
    void HandleLoadoutChanged();

    // The loadout the actions component was last built from
    UPROPERTY()
    TObjectPtr<UActionLoadoutDataAsset> ActiveLoadout = nullptr;

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
