// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnBasedMechanicsStructs.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"


bool FTurnParticipantInfo::IsActiveParticipant() const
{
	if (!bConnected) return false;

	const ATurnBasedPlayerState* PS =
		Cast<ATurnBasedPlayerState>(PlayerState);

	return IsValid(PS) && !PS->IsForfeited();
}

FString FTurnParticipantInfo::GetDisplayName() const
{
	return IsValid(PlayerState)
			? PlayerState->GetPlayerName()
			: TEXT("Unknown");
}
