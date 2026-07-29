// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/GameState/TurnBasedGameState.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"
#include "Net/UnrealNetwork.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"


ATurnBasedGameState::ATurnBasedGameState()
{
    ParticipantManager =
        CreateDefaultSubobject<UTurnBasedParticipantManagerComponent>(
            TEXT("ParticipantManager"));
}

void ATurnBasedGameState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATurnBasedGameState, MatchPhase);
}

// --- Match Phase ---

void ATurnBasedGameState::SetMatchPhase(EMatchPhase NewPhase)
{
    if (!HasAuthority()) return;
    if (MatchPhase == NewPhase) return;

    MatchPhase = NewPhase;

    // Fire on server immediately -- clients receive via OnRep
    BroadcastMatchPhaseChanged();

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedGameState: Match phase -> %s"),
        *UEnum::GetValueAsString(NewPhase));
}

void ATurnBasedGameState::OnRep_MatchPhase()
{
    BroadcastMatchPhaseChanged();
}

void ATurnBasedGameState::BroadcastMatchPhaseChanged()
{
    OnMatchPhaseChanged.Broadcast(MatchPhase);
    OnMatchPhaseChanged_Native.Broadcast(MatchPhase);
}

// --- Passthrough Accessors ---

ETurnPhase ATurnBasedGameState::GetCurrentTurnPhase() const
{
    return IsValid(ParticipantManager)
        ? ParticipantManager->CurrentPhase
        : ETurnPhase::TurnStart;
}

int32 ATurnBasedGameState::GetActiveParticipantIndex() const
{
    return IsValid(ParticipantManager)
        ? ParticipantManager->ActiveParticipantIndex
        : INDEX_NONE;
}

int32 ATurnBasedGameState::GetActiveTurnNumber() const
{
    return IsValid(ParticipantManager)
        ? ParticipantManager->TurnNumber
        : 0;
}

const TArray<FTurnParticipantInfo>&
ATurnBasedGameState::GetParticipants() const
{
    static TArray<FTurnParticipantInfo> Empty;
    return IsValid(ParticipantManager)
        ? ParticipantManager->Participants
        : Empty;
}

float ATurnBasedGameState::GetTurnDuration() const
{
    return IsValid(ParticipantManager)
        ? ParticipantManager->TurnDuration
        : 0.f;
}