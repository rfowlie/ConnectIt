// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/GameState/TurnBasedGameState.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"


ATurnBasedGameState::ATurnBasedGameState()
{
	TurnManager = CreateDefaultSubobject<UTurnBasedParticipantManagerComponent>(
		TEXT("TurnManager"));
}

ETurnPhase ATurnBasedGameState::GetCurrentPhase() const
{
	return IsValid(TurnManager)
		? TurnManager->CurrentPhase
		: ETurnPhase::WaitingForParticipants;
}

int32 ATurnBasedGameState::GetActiveTurnNumber() const
{
	return IsValid(TurnManager) ? TurnManager->TurnNumber : 0;
}

const TArray<FTurnParticipantInfo>& ATurnBasedGameState::GetParticipants() const
{
	static TArray<FTurnParticipantInfo> Empty;
	return IsValid(TurnManager) ? TurnManager->Participants : Empty;
}

float ATurnBasedGameState::GetTurnDuration() const
{
	return IsValid(TurnManager) ? TurnManager->TurnDuration : 0.f;
}
