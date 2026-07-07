// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/GameMode/TurnBasedGameMode.h"
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

    UTurnBasedParticipantManagerComponent* Manager = GetTurnManager();
    if (!Manager) return;

    // Check if this is a reconnect
    const FString DisplayName = NewPlayer->GetPlayerState<APlayerState>()
        ? NewPlayer->GetPlayerState<APlayerState>()->GetPlayerName()
        : TEXT("Player");

    // Check if already registered (reconnect case)
    if (FTurnParticipantInfo* Existing = Manager->FindParticipant(NewPlayer))
    {
        Manager->NotifyParticipantReconnected(NewPlayer);
        return;
    }

    // New participant — register
    Manager->RegisterParticipant(NewPlayer, EParticipantType::Human, DisplayName);

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedGameMode: Human participant %s logged in"),
        *DisplayName);
}

void ATurnBasedGameMode::Logout(AController* Exiting)
{
    UTurnBasedParticipantManagerComponent* Manager = GetTurnManager();
    if (Manager)
    {
        Manager->NotifyParticipantDisconnected(Exiting);
    }

    Super::Logout(Exiting);
}

void ATurnBasedGameMode::HandleMatchHasStarted()
{
    Super::HandleMatchHasStarted();

    UTurnBasedParticipantManagerComponent* Manager = GetTurnManager();
    if (!Manager) return;

    // Apply configuration
    Manager->TurnDuration      = TurnDuration;
    Manager->ForfeitThreshold  = ForfeitThreshold;
    Manager->ReconnectTimeout  = ReconnectTimeout;
}

void ATurnBasedGameMode::RegisterAIParticipant(
    AController* AIController,
    const FString& DisplayName)
{
    UTurnBasedParticipantManagerComponent* Manager = GetTurnManager();
    if (!Manager) return;

    Manager->RegisterParticipant(AIController, EParticipantType::AI, DisplayName);
}

void ATurnBasedGameMode::StartReadyCheck()
{
    UTurnBasedParticipantManagerComponent* Manager = GetTurnManager();
    if (!Manager) return;

    Manager->BeginReadyCheck();
}

UTurnBasedParticipantManagerComponent* ATurnBasedGameMode::GetTurnManager() const
{
    ATurnBasedGameState* GS = GetGameState<ATurnBasedGameState>();
    return IsValid(GS) ? GS->TurnManager : nullptr;
}
