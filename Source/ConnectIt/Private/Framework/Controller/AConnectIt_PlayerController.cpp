// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/ConnectIt_BoardManager.h"
#include "Framework/Controller/ConnectIt_PlayerController.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"
#include "Action/TurnBasedActionsComponent.h"
#include "Framework/Data/ConnectIt_ConfigComponent.h"



void AConnectIt_PlayerController::BeginPlay()
{
    // Base creates ParticipantComponent/ActionsComponent, wires them
    // together (and to game state match phase changes) via
    // CoordinatorComponent, and notifies ready
    Super::BeginPlay();

    // Only owning client needs actions and input wiring
    if (!IsLocalController()) return;

    // Wire action component delegate -- board change routing is
    // ConnectIt-specific and not handled by the generic coordinator
    ActionsComponent->OnBoardChangeRequested.AddDynamic(
        this,
        &AConnectIt_PlayerController::HandleBoardChangeRequested);

    // Initialise from board manager
    InitialiseFromBoardManager();
}

// --- Initialisation ---

void AConnectIt_PlayerController::InitialiseFromBoardManager()
{
    CachedBoardManager =
        UConnectIt_GameUtilityLibrary::GetBoardManager(this);

    if (!IsValid(CachedBoardManager))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PlayerController: No AConnectIt_BoardManager "
                 "found in world"));
        return;
    }

    // Get player loadout from board manager config
    UActionLoadoutDataAsset* Loadout =
        CachedBoardManager->GetConfigComponent()->PlayerLoadout;

    if (!IsValid(Loadout))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PlayerController: No player loadout set "
                 "on board manager config"));
        return;
    }

    ActionsComponent->InitialiseFromLoadout(Loadout);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_PlayerController: Initialised from board manager "
             "with loadout '%s'"),
        *Loadout->LoadoutName);
}

// --- Action Component Handler ---

// required to satisfy delegate signature and provide moment to add logic before sending request
void AConnectIt_PlayerController::HandleBoardChangeRequested(
    const FTurnActionRequest& Request)
{
    // Route to server via RPC
    // Server validates and passes to board manager
    ServerRouteBoardChangeRequest(Request);
}

// --- ServerRPC ---

void AConnectIt_PlayerController::ServerRouteBoardChangeRequest_Implementation(
    FTurnActionRequest Request)
{
    // Server side -- validate it is this player's turn. NOT
    // ParticipantComponent->IsMyTurn() -- that flag is client-side-only
    // (set solely inside a Client RPC) and is always false on the server.
    // UTurnBasedParticipantManagerComponent::IsActiveParticipant is the
    // server-authoritative equivalent (compares against the replicated
    // ActiveParticipantIndex, same pattern ServerSubmitTurnEnd already uses).
    UTurnBasedParticipantManagerComponent* ParticipantManager =
        ParticipantComponent->GetParticipantManager();

    if (!IsValid(ParticipantManager) || !ParticipantManager->IsActiveParticipant(this)
        || ParticipantManager->IsMatchOver())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_PlayerController: Board change request "
                 "rejected — not this participant's turn, or match "
                 "already over"));
        ClientNotifyBoardChangeOutcome(Request, false);
        return;
    }

    // TODO: this feels problematic, any action can be made valid if no FactionID set?
    // TODO: should this bo added to the failure case?
    // Stamp faction ID if not set by action
    if (Request.FactionID < 0)
    {
        Request.FactionID = ParticipantComponent->GetActiveParticipantSlotIndex();
    }

    // Route to board manager component
    AConnectIt_BoardManager* BoardManager =
        UConnectIt_GameUtilityLibrary::GetBoardManager(this);

    if (!IsValid(BoardManager))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PlayerController: Cannot route board change "
                 "— board manager component not found"));
        ClientNotifyBoardChangeOutcome(Request, false);
        return;
    }

    // pass to board manager on the server
    const bool bSucceeded = BoardManager->ProcessRequest(Request);

    // return success value to client to halt player input
    ClientNotifyBoardChangeOutcome(Request, bSucceeded);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_PlayerController: Board change request routed "
             "'%s' from faction %d — %s"),
        *Request.RequestType.ToString(),
        Request.FactionID,
        bSucceeded ? TEXT("succeeded") : TEXT("rejected"));
}

// --- ClientRPC ---

void AConnectIt_PlayerController::ClientNotifyBoardChangeOutcome_Implementation(
    FTurnActionRequest Request, bool bSucceeded)
{
    if (!IsValid(ActionsComponent))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PlayerController: ClientNotifyBoardChangeOutcome "
                 "— ActionsComponent is null"));
        return;
    }

    ActionsComponent->NotifyBoardChangeOutcome(Request, bSucceeded);
}
