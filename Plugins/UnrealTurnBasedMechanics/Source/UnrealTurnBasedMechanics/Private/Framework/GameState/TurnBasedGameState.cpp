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
    // -1 indicates error
    return IsValid(ParticipantManager)
        ? ParticipantManager->ActiveParticipantIndex
        : -1;
}

int32 ATurnBasedGameState::GetActiveTurnNumber() const
{
    // -1 indicates error
    return IsValid(ParticipantManager)
        ? ParticipantManager->TurnNumber
        : -1;
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

FTurnParticipantInfo ATurnBasedGameState::GetActiveParticipant(bool& bOutValid) const
{
    if (IsValid(ParticipantManager))
    {
        return ParticipantManager->GetActiveParticipant(bOutValid);
    }

    bOutValid = false;
    return FTurnParticipantInfo();
}

FTurnParticipantInfo ATurnBasedGameState::GetParticipantBySlot(
    int32 InSlotIndex, bool& bOutValid) const
{
    if (IsValid(ParticipantManager))
    {
        return ParticipantManager->GetParticipantBySlot(InSlotIndex, bOutValid);
    }

    bOutValid = false;
    return FTurnParticipantInfo();
}

float ATurnBasedGameState::GetTurnTimeRemaining(bool& bOutTimerRunning) const
{
    if (!IsValid(ParticipantManager)
        || ParticipantManager->ReplicatedTurnStartServerTime < 0.f)
    {
        bOutTimerRunning = false;
        return 0.f;
    }

    const float Elapsed = GetServerWorldTimeSeconds()
        - ParticipantManager->ReplicatedTurnStartServerTime;
    const float Remaining = ParticipantManager->ReplicatedTurnDuration - Elapsed;

    bOutTimerRunning = true;
    return FMath::Max(Remaining, 0.f);
}

float ATurnBasedGameState::GetTurnTimeRemainingFraction() const
{
    if (!IsValid(ParticipantManager) || ParticipantManager->ReplicatedTurnDuration <= 0.f)
    {
        return 0.f;
    }

    bool bTimerRunning = false;
    const float Remaining = GetTurnTimeRemaining(bTimerRunning);
    if (!bTimerRunning) return 0.f;

    return FMath::Clamp(Remaining / ParticipantManager->ReplicatedTurnDuration, 0.f, 1.f);
}