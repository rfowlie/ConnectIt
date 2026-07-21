// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Controller/TurnBasedAIController.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "Action/TurnBasedActionsComponent.h"


ATurnBasedAIController::ATurnBasedAIController()
{
    ParticipantComponent =
        CreateDefaultSubobject<UTurnBasedParticipantComponent>(
            TEXT("ParticipantComponent"));

    ActionsComponent =
        CreateDefaultSubobject<UTurnBasedActionsComponent>(
            TEXT("ActionsComponent"));
}

void ATurnBasedAIController::BeginPlay()
{
    Super::BeginPlay();

    // AI controllers only exist on the server
    if (!HasAuthority()) return;

    // PlayerState is required for participant registration
    // AI controllers do not create one by default
    if (bAutoCreatePlayerState)
    {
        EnsurePlayerState();
    }

    BindParticipantDelegates();
}

void ATurnBasedAIController::EnsurePlayerState()
{
    if (!HasAuthority()) return;

    // Already has one -- nothing to do
    if (IsValid(PlayerState))
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("TurnBasedAIControllerBase: %s already has PlayerState"),
            *GetName());
        return;
    }

    // Creates PlayerState using GameMode's PlayerStateClass
    InitPlayerState();

    if (!IsValid(PlayerState))
    {
        UE_LOG(LogTemp, Error,
            TEXT("TurnBasedAIControllerBase: %s failed to create "
                 "PlayerState. Check that GameMode has a valid "
                 "PlayerStateClass set."),
            *GetName());
        return;
    }

    // Configure for AI
    PlayerState->SetPlayerName(AIDisplayName);
    PlayerState->SetIsABot(true);

    // Verify it is the correct type for turn-based participation
    if (!PlayerState->IsA<ATurnBasedPlayerState>())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedAIControllerBase: %s PlayerState is not "
                 "ATurnBasedPlayerState. Participant registration "
                 "will fail. Set GameMode PlayerStateClass to "
                 "ATurnBasedPlayerState or a subclass."),
            *GetName());
        return;
    }

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedAIControllerBase: %s PlayerState created — '%s'"),
        *GetName(),
        *AIDisplayName);
}

void ATurnBasedAIController::BindParticipantDelegates()
{
    if (!IsValid(ParticipantComponent)) return;

    ParticipantComponent->OnTurnNotificationReceived.AddDynamic(
        this,
        &ATurnBasedAIController::OnTurnNotificationReceived);

    ParticipantComponent->OnOpponentTurnStarted.AddDynamic(
        this,
        &ATurnBasedAIController::OnOpponentTurnStartedReceived);
}

// --- Delegate Forwarders ---

void ATurnBasedAIController::OnTurnNotificationReceived(
    const FTurnNotification& Notification)
{
    HandleTurnNotification(Notification);
}

void ATurnBasedAIController::OnOpponentTurnStartedReceived(
    int32 ActiveParticipantSlotIndex)
{
    HandleOpponentTurnStarted(ActiveParticipantSlotIndex);
}

// --- Default Handlers ---

void ATurnBasedAIController::HandleTurnNotification_Implementation(
    const FTurnNotification& Notification)
{
    // Default handles action component lifecycle
    // Subclasses call Super then add their AI decision-making

    if (!IsValid(ActionsComponent)) return;

    switch (Notification.Phase)
    {
        case ETurnPhase::TurnStart:
        case ETurnPhase::TurnActive:
            ActionsComponent->NotifyTurnStarted(Notification.TurnNumber);
            break;

        case ETurnPhase::TurnEnd:
        case ETurnPhase::TurnTimeout:
        case ETurnPhase::TurnSkipped:
            ActionsComponent->NotifyTurnEnded();
            break;

        default:
            break;
    }
}

void ATurnBasedAIController::HandleOpponentTurnStarted_Implementation(
    int32 ActiveParticipantSlotIndex)
{
    if (!IsValid(ActionsComponent)) return;
    ActionsComponent->NotifyOpponentTurnStarted();
}

