// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Controller/ConnectIt_PlayerController.h"
#include "EngineUtils.h"
#include "Board/ConnectIt_BoardManager.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Action/TurnBasedActionsComponent.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"


AConnectIt_PlayerController::AConnectIt_PlayerController()
{
    ParticipantComponent =
        CreateDefaultSubobject<UTurnBasedParticipantComponent>(
            TEXT("ParticipantComponent"));

    ActionComponent =
        CreateDefaultSubobject<UTurnBasedActionsComponent>(
            TEXT("ActionComponent"));
}

void AConnectIt_PlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Bind turn notifications
    ParticipantComponent->OnTurnNotificationReceived.AddDynamic(
        this, &AConnectIt_PlayerController::HandleTurnChanged);
    

    // Initialise from board actor
    // Only owning client needs actions -- IsLocalController guards this
    if (IsLocalController())
    {
        InitialiseFromBoardActor();
    }
}

void AConnectIt_PlayerController::InitialiseFromBoardActor()
{
    AConnectIt_BoardManager* BoardActor = UConnectIt_GameUtilityLibrary::GetBoardManager(this);

    if (!IsValid(BoardActor))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectItPlayerController: No AConnectItBoardActor "
                 "found in world"));
        return;
    }

    // Initialise action component from player loadout
    UActionLoadOutDataAsset* Loadout = BoardActor->GetPlayerLoadout();
    if (!IsValid(Loadout))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectItPlayerController: No player loadout set "
                 "on board actor config"));
        return;
    }

    ActionComponent->InitialiseFromLoadout(Loadout);

    // Wire action component passthrough to board manager
    // OnBoardChangeRequested fires on SERVER inside ServerRPC
    // Project wiring connects it to the board manager here
    ActionComponent->OnBoardChangeRequested.AddDynamic(
        BoardActor->BoardManager,
        &UConnectIt_BoardManagerComponent::ProcessRequest);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectItPlayerController: Initialised from board actor "
             "with loadout '%s'"),
        *Loadout->LoadoutName);
}

void AConnectIt_PlayerController::HandleTurnChanged(const FTurnNotification& Notification)
{
    switch (Notification.Phase)
    {
        case ETurnPhase::TurnStart :
            ActionComponent->NotifyTurnStarted(Notification.TurnNumber);
            break;
        case ETurnPhase::TurnEnd :
            ActionComponent->NotifyTurnEnded();
            break;
        default:
            break;
    }
    
    UE_LOG(LogTemp, Log,
        TEXT("ConnectItPlayerController: Turn %d started"),
        Notification.TurnNumber);
}