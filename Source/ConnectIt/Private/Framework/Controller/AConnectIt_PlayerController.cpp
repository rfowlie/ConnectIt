// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Controller/AConnectIt_PlayerController.h"
#include "EngineUtils.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Action/TurnBasedActionComponent.h"
#include "Board/ConnectItBoardActor.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"


AConnectIt_PlayerController::AConnectIt_PlayerController()
{
    ParticipantComponent =
        CreateDefaultSubobject<UTurnBasedParticipantComponent>(
            TEXT("ParticipantComponent"));

    ActionComponent =
        CreateDefaultSubobject<UTurnBasedActionComponent>(
            TEXT("ActionComponent"));
}

void AConnectIt_PlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Bind turn notifications
    ParticipantComponent->OnTurnStarted.AddDynamic(
        this, &AConnectIt_PlayerController::HandleTurnStarted);

    ParticipantComponent->OnTurnEnded.AddDynamic(
        this, &AConnectIt_PlayerController::HandleTurnEnded);

    // Initialise from board actor
    // Only owning client needs actions -- IsLocalController guards this
    if (IsLocalController())
    {
        InitialiseFromBoardActor();
    }
}

void AConnectIt_PlayerController::InitialiseFromBoardActor()
{
    AConnectItBoardActor* BoardActor = nullptr;
    for (TActorIterator<AConnectItBoardActor> It(GetWorld()); It; ++It)
    {
        BoardActor = *It;
        break;
    }

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
        &UConnectItBoardManagerComponent::ProcessRequest);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectItPlayerController: Initialised from board actor "
             "with loadout '%s'"),
        *Loadout->LoadoutName);
}

void AConnectIt_PlayerController::HandleTurnStarted(
    const FTurnNotification& Notification)
{
    // Notify action component to reset state and activate required actions
    ActionComponent->OnTurnBegan(Notification.TurnNumber);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectItPlayerController: Turn %d started"),
        Notification.TurnNumber);
}

void AConnectIt_PlayerController::HandleTurnEnded(
    const FTurnNotification& Notification)
{
    UE_LOG(LogTemp, Log,
        TEXT("ConnectItPlayerController: Turn %d ended — reason: %s"),
        Notification.TurnNumber,
        *UEnum::GetValueAsString(Notification.EndReason));
}