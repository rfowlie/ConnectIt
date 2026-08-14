// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/GameMode/TurnBasedGameMode.h"
#include "UnrealTurnBasedMechanics.h"
#include "Framework/GameState/TurnBasedGameState.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"


ATurnBasedGameMode::ATurnBasedGameMode()
{
    GameStateClass  = ATurnBasedGameState::StaticClass();
    PlayerStateClass = ATurnBasedPlayerState::StaticClass();
}

void ATurnBasedGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    UTurnBasedParticipantManagerComponent* Manager = GetParticipantManager();
    if (!Manager) return;
    
    // Check if already registered (reconnect case)
    if (Manager->IsParticipantRegistered(NewPlayer))
    {
        Manager->NotifyParticipantReconnected(NewPlayer);
        return;
    }

    // New participant — register
    Manager->RegisterParticipant(NewPlayer, EParticipantType::Human);

    // Check if this is a reconnect
    const FString DisplayName = NewPlayer->GetPlayerState<APlayerState>()
        ? NewPlayer->GetPlayerState<APlayerState>()->GetPlayerName()
        : TEXT("Player");
    
    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedGameMode: Human participant %s logged in"),
        *DisplayName);
}

void ATurnBasedGameMode::Logout(AController* Exiting)
{
    UTurnBasedParticipantManagerComponent* Manager = GetParticipantManager();
    if (Manager)
    {
        Manager->NotifyParticipantDisconnected(Exiting);
    }

    Super::Logout(Exiting);
}

void ATurnBasedGameMode::HandleMatchHasStarted()
{
    Super::HandleMatchHasStarted();

    UTurnBasedParticipantManagerComponent* Manager = GetParticipantManager();
    if (!Manager) return;

    // Apply configuration
    Manager->TurnDuration     = TurnDuration;
    Manager->ForfeitThreshold = ForfeitThreshold;
    Manager->ReconnectTimeout = ReconnectTimeout;
}

void ATurnBasedGameMode::RegisterAIParticipant(
    AController* AIController,
    const FString& DisplayName) const
{
    UTurnBasedParticipantManagerComponent* Manager = GetParticipantManager();
    if (!Manager) return;

    if (Manager->IsParticipantRegistered(AIController))
    {
        Manager->NotifyParticipantReconnected(AIController);
        return;
    }
    
    Manager->RegisterParticipant(AIController, EParticipantType::AI);

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedGameMode: AI participant %s registered"),
        *DisplayName);
}

void ATurnBasedGameMode::StartReadyCheck()
{
    UTurnBasedParticipantManagerComponent* Manager = GetParticipantManager();
    if (!Manager) return;

    Manager->BeginReadyCheck();
}

UTurnBasedParticipantManagerComponent* ATurnBasedGameMode::GetParticipantManager() const
{
    const ATurnBasedGameState* Gs = GetGameState<ATurnBasedGameState>();
    return IsValid(Gs) ? Gs->ParticipantManager : nullptr;
}
