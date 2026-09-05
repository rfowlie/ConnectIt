// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Controller/ConnectIt_PlayerController.h"
#include "Framework/GameMode/ConnectIt_GameMode.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Action/TurnBasedActionsComponent.h"
#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Piece/GridPieceRegistryBase.h"
#include "Tile/GridTileRegistryBase.h"



void AConnectIt_PlayerController::BeginPlay()
{
    // Base creates ParticipantComponent/ActionsComponent, wires them
    // together (and to game state match phase changes) via
    // CoordinatorComponent, and notifies ready
    Super::BeginPlay();

    // Local tile/piece discovery -- runs unconditionally on every instance
    // of this controller (the owning client's local one AND the server's
    // own proxy for it), same as ABoardManagerBase used to do for these
    // registries before they moved here. AConnectIt_BoardManager::
    // InitialiseBoard relies on at least one connected controller having
    // already discovered tiles by the time it runs.
    if (IsValid(TileRegistry))
    {
        TileRegistry->InitialiseRegistry();
    }
    if (IsValid(PieceRegistry))
    {
        PieceRegistry->InitialiseRegistry();
    }

    // Only owning client needs actions and input wiring
    if (!IsLocalController()) return;

    // Wire action component delegate -- board change routing is
    // ConnectIt-specific and not handled by the generic coordinator
    ActionsComponent->OnBoardChangeRequested.AddDynamic(
        this,
        &AConnectIt_PlayerController::HandleBoardChangeRequested);

    // Initialise from level config
    InitialiseFromLevelConfig();
}

void AConnectIt_PlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(TileRegistry))
    {
        TileRegistry->ShutdownRegistry();
    }

    Super::EndPlay(EndPlayReason);
}

// --- Initialisation ---

void AConnectIt_PlayerController::InitialiseFromLevelConfig()
{
    const UConnectIt_LevelConfigDataAsset* LevelConfig =
        UConnectIt_GameUtilityLibrary::GetLevelConfig(this);

    if (!IsValid(LevelConfig))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PlayerController: No ConnectIt_LevelConfigDataAsset "
                 "found for the current level"));
        return;
    }

    UActionLoadoutDataAsset* Loadout = LevelConfig->PlayerLoadout;

    if (!IsValid(Loadout))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PlayerController: No PlayerLoadout set "
                 "on the level config"));
        return;
    }

    ActionsComponent->InitialiseFromLoadout(Loadout);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_PlayerController: Initialised from level config "
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

    // Route to GameMode -- server-only, structurally unreachable from any
    // client, so no defensive HasAuthority() check is needed here beyond
    // what this being a Server RPC's _Implementation already guarantees.
    AConnectIt_GameMode* GameMode = GetWorld()->GetAuthGameMode<AConnectIt_GameMode>();

    if (!IsValid(GameMode))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PlayerController: Cannot route board change "
                 "— GameMode is not AConnectIt_GameMode"));
        ClientNotifyBoardChangeOutcome(Request, false);
        return;
    }

    const bool bSucceeded = GameMode->ProcessBoardRequest(Request);

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
