// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnBasedMechanicsStructs.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "TurnBasedMechanicsLibrary.h"


bool FTurnParticipantInfo::IsActiveParticipant() const
{
	if (!bConnected) return false;

	const ATurnBasedPlayerState* PS =
		Cast<ATurnBasedPlayerState>(PlayerState);

	return IsValid(PS) && !PS->IsForfeited();
}

FString FTurnParticipantInfo::GetDisplayName() const
{
	// USTRUCT member functions can't be UFUNCTIONs, so the Blueprint-callable
	// version of this lives on UTurnBasedMechanicsLibrary; delegate to it so
	// there's exactly one implementation.
	return UTurnBasedMechanicsLibrary::GetParticipantDisplayName(*this);
}
