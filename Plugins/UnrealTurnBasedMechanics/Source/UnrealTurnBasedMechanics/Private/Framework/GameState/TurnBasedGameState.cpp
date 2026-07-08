// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/GameState/TurnBasedGameState.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"


ATurnBasedGameState::ATurnBasedGameState()
{
	ParticipantManager = CreateDefaultSubobject<UTurnBasedParticipantManagerComponent>(
		TEXT("TurnManager"));
}

const TArray<FTurnParticipantInfo>& ATurnBasedGameState::GetParticipants() const
{
	static TArray<FTurnParticipantInfo> Empty;
	return IsValid(ParticipantManager) ? ParticipantManager->Participants : Empty;
}

float ATurnBasedGameState::GetTurnDuration() const
{
	return IsValid(ParticipantManager) ? ParticipantManager->TurnDuration : 0.f;
}

ETurnPhase ATurnBasedGameState::GetCurrentPhase() const
{
	return IsValid(ParticipantManager)
		? ParticipantManager->CurrentPhase
		: ETurnPhase::WaitingForParticipants;
}

int32 ATurnBasedGameState::GetActiveTurnNumber() const
{
	return IsValid(ParticipantManager) ? ParticipantManager->TurnNumber : 0;
}