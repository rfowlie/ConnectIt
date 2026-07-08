// Fill out your copyright notice in the Description page of Project Settings.


#include "Turn/Order/RandomTurnOrderStrategy.h"


int32 URandomTurnOrderStrategy::GetNextParticipantIndex_Implementation(
	const TArray<FTurnParticipantInfo>& Participants,
	int32 CurrentIndex)
{
	TArray<int32> Active;
	for (int32 i = 0; i < Participants.Num(); i++)
	{
		if (Participants[i].IsActiveParticipant())
			Active.Add(i);
	}

	if (Active.IsEmpty()) return -1;
	return Active[FMath::RandRange(0, Active.Num() - 1)];
}

int32 URandomTurnOrderStrategy::GetFirstParticipantIndex_Implementation(
	const TArray<FTurnParticipantInfo>& Participants)
{
	return GetNextParticipantIndex_Implementation(Participants, -1);
}

FString URandomTurnOrderStrategy::GetStrategyName_Implementation()
{
	return TEXT("Random");
}