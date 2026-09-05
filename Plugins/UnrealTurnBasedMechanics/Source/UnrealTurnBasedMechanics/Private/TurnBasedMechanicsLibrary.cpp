// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnBasedMechanicsLibrary.h"

#include "Framework/GameState/TurnBasedGameState.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"


bool UTurnBasedMechanicsLibrary::IsLocalPlayerActiveParticipant(const UObject* WorldContextObject)
{
	if (!IsValid(WorldContextObject)) return false;
	
	const UWorld* World = WorldContextObject->GetWorld();
	if (!IsValid(World)) return false;
	
	const APlayerController* LocalPC = World->GetFirstPlayerController();
	if (!IsValid(LocalPC)) return false;

	const UTurnBasedParticipantComponent* ParticipantComp =
		LocalPC->FindComponentByClass<UTurnBasedParticipantComponent>();
	
	if (!IsValid(ParticipantComp)) return false;

	return ParticipantComp->IsMyTurn();

	// const ATurnBasedGameState* GameState = World->GetGameState<ATurnBasedGameState>();
	//
	// if (!IsValid(GameState)) return false;
	//
	// return GameState->GetActiveParticipantIndex() == ParticipantComp->GetActiveParticipantSlotIndex();
}

FString UTurnBasedMechanicsLibrary::GetParticipantDisplayName(
	const FTurnParticipantInfo& Participant,
	const FString& FallbackName)
{
	return IsValid(Participant.PlayerState)
			? Participant.PlayerState->GetPlayerName()
			: FallbackName;
}