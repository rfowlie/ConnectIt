// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Controller/TurnBasedAIController.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "Action/TurnBasedActionsComponent.h"
#include "Framework/Controller/TurnBasedControllerCoordinatorComponent.h"


ATurnBasedAIController::ATurnBasedAIController()
{
    ParticipantComponent =
        CreateDefaultSubobject<UTurnBasedParticipantComponent>(
            TEXT("ParticipantComponent"));

    ActionsComponent =
        CreateDefaultSubobject<UTurnBasedActionsComponent>(
            TEXT("ActionsComponent"));

    CoordinatorComponent =
        CreateDefaultSubobject<UTurnBasedControllerCoordinatorComponent>(
            TEXT("CoordinatorComponent"));
}

void ATurnBasedAIController::BeginPlay()
{
    // PlayerState must exist before coordinator wiring
    // and before participant registration
    if (HasAuthority() && bAutoCreatePlayerState)
    {
        EnsurePlayerState();
    }

    // Super triggers component BeginPlay including coordinator wiring
    Super::BeginPlay();
}

void ATurnBasedAIController::EnsurePlayerState()
{
    if (!HasAuthority()) return;
    if (IsValid(PlayerState)) return;

    InitPlayerState();

    if (!IsValid(PlayerState))
    {
        UE_LOG(LogTemp, Error,
            TEXT("TurnBasedAIControllerBase: %s failed to create "
                 "PlayerState. Check GameMode PlayerStateClass."),
            *GetName());
        return;
    }

    PlayerState->SetPlayerName(AIDisplayName);
    PlayerState->SetIsABot(true);

    if (!PlayerState->IsA<ATurnBasedPlayerState>())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedAIControllerBase: %s PlayerState is not "
                 "ATurnBasedPlayerState. Registration will fail."),
            *GetName());
        return;
    }

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedAIControllerBase: %s PlayerState created — '%s'"),
        *GetName(), *AIDisplayName);
}