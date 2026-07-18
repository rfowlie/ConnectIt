// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/ConnectIt_BoardManager.h"
#include "Framework/Controller/ConnectIt_PlayerController.h"
#include "Framework/GameState/ConnectIt_GameState.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"


AConnectIt_PlayerController::AConnectIt_PlayerController()
{
    ParticipantComponent =
        CreateDefaultSubobject<UTurnBasedParticipantComponent>(
            TEXT("ParticipantComponent"));

    ActionsComponent =
        CreateDefaultSubobject<UTurnBasedActionsComponent>(
            TEXT("ActionsComponent"));
}

void AConnectIt_PlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Only owning client needs actions and input wiring
    if (!IsLocalController()) return;

    // Wire participant component delegates
    ParticipantComponent->OnTurnNotificationReceived.AddDynamic(
        this,
        &AConnectIt_PlayerController::HandleTurnNotificationReceived);

    ParticipantComponent->OnOpponentTurnStarted.AddDynamic(
        this,
        &AConnectIt_PlayerController::HandleOpponentTurnStarted);

    // Wire game state match phase delegate
    if (AConnectIt_GameState* GS =
        GetWorld()->GetGameState<AConnectIt_GameState>())
    {
        GS->OnMatchPhaseChanged.AddDynamic(
            this,
            &AConnectIt_PlayerController::HandleMatchPhaseChanged);
    }

    // Wire action component delegates
    ActionsComponent->OnBoardChangeRequested.AddDynamic(
        this,
        &AConnectIt_PlayerController::HandleBoardChangeRequested);

    ActionsComponent->OnTurnEndRequested.AddDynamic(
        this,
        &AConnectIt_PlayerController::HandleTurnEndRequested);

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
    UActionLoadOutDataAsset* Loadout =
        CachedBoardManager->GetPlayerLoadout();

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

// --- Participant Delegate Handlers ---

void AConnectIt_PlayerController::HandleTurnNotificationReceived(
    const FTurnNotification& Notification)
{
    switch (Notification.Phase)
    {
        case ETurnPhase::TurnStart:
        case ETurnPhase::TurnActive:
            // Our turn -- notify action component
            ActionsComponent->NotifyTurnStarted(Notification.TurnNumber);

            UE_LOG(LogTemp, Log,
                TEXT("ConnectIt_PlayerController: Turn %d started"),
                Notification.TurnNumber);
            break;

        case ETurnPhase::TurnEnd:
        case ETurnPhase::TurnTimeout:
        case ETurnPhase::TurnSkipped:
            // Our turn ended -- push idle viewer
            ActionsComponent->NotifyTurnEnded();

            UE_LOG(LogTemp, Log,
                TEXT("ConnectIt_PlayerController: Turn ended — reason: %s"),
                *UEnum::GetValueAsString(Notification.EndReason));
            break;

        case ETurnPhase::TurnPaused:
            // Handled by match phase delegate
            break;

        default:
            break;
    }
}

void AConnectIt_PlayerController::HandleOpponentTurnStarted(
    int32 ActiveParticipantSlotIndex)
{
    // Opponent turn starting -- push spectator viewer
    ActionsComponent->NotifyOpponentTurnStarted();

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_PlayerController: Opponent turn started "
             "— active slot: %d"),
        ActiveParticipantSlotIndex);
}

// --- Match Phase Handler ---

void AConnectIt_PlayerController::HandleMatchPhaseChanged(EMatchPhase NewPhase)
{
    switch (NewPhase)
    {
    case EMatchPhase::Paused:
        bIsPaused = true;
        ActionsComponent->NotifyPaused();
        break;

    case EMatchPhase::InProgress:
        if (bIsPaused)
        {
            bIsPaused = false;
            ActionsComponent->NotifyUnpaused();
        }
        break;

    case EMatchPhase::GameOver:
        ActionsComponent->NotifyMatchEnded();
        break;

    default:
        break;
    }
}

// --- Action Component Handlers ---

void AConnectIt_PlayerController::HandleBoardChangeRequested(
    const FTurnActionRequest& Request)
{
    // Route to server via RPC
    // Server validates and passes to board manager
    ServerRouteBoardChangeRequest(Request);
}

void AConnectIt_PlayerController::HandleTurnEndRequested()
{
    // Route to participant component ServerRPC
    ParticipantComponent->ServerSubmitTurnEnd();

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_PlayerController: Turn end submitted"));
}

// --- ServerRPC ---

void AConnectIt_PlayerController::ServerRouteBoardChangeRequest_Implementation(
    FTurnActionRequest Request)
{
    // Server side -- validate it is this player's turn
    if (!ParticipantComponent->IsMyTurn())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_PlayerController: Board change request "
                 "rejected — not this participant's turn"));
        return;
    }

    // Stamp faction ID if not set by action
    if (Request.FactionID < 0)
    {
        Request.FactionID = ParticipantComponent->CachedSlotIndex;
    }

    // Route to board manager component
    AConnectIt_BoardManager* BoardManager =
        UConnectIt_GameUtilityLibrary::GetBoardManager(this);

    if (!IsValid(BoardManager))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PlayerController: Cannot route board change "
                 "— board manager component not found"));
        return;
    }

    BoardManager->ProcessRequest(Request);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_PlayerController: Board change request routed "
             "'%s' from faction %d"),
        *Request.RequestType.ToString(),
        Request.FactionID);
}