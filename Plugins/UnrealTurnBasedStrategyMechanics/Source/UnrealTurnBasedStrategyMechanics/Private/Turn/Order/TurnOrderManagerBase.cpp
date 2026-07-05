// Fill out your copyright notice in the Description page of Project Settings.


#include "Turn/Order/TurnOrderManagerBase.h"

int32 UTurnOrderManagerBase::GetNextParticipantIndex_Implementation(
	const TArray<FTurnParticipantInfo>& Participants, int32 CurrentIndex)
{
	return ITurnOrderInterface::GetNextParticipantIndex_Implementation(Participants, CurrentIndex);
}

int32 UTurnOrderManagerBase::GetFirstParticipantIndex_Implementation(const TArray<FTurnParticipantInfo>& Participants)
{
	return ITurnOrderInterface::GetFirstParticipantIndex_Implementation(Participants);
}

FString UTurnOrderManagerBase::GetStrategyName_Implementation()
{
	return ITurnOrderInterface::GetStrategyName_Implementation();
}
