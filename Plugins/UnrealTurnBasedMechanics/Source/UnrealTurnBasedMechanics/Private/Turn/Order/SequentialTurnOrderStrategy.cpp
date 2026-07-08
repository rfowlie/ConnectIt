// Fill out your copyright notice in the Description page of Project Settings.


#include "Turn/Order/SequentialTurnOrderStrategy.h"


int32 USequentialTurnOrderStrategy::GetNextParticipantIndex_Implementation(
	const TArray<FTurnParticipantInfo>& Participants,
	int32 CurrentIndex)
{
	if (Participants.IsEmpty()) return -1;

	const int32 Count = Participants.Num();
	for (int32 Offset = 1; Offset <= Count; Offset++)
	{
		const int32 NextIndex = (CurrentIndex + Offset) % Count;
		if (Participants[NextIndex].IsActiveParticipant())
			return NextIndex;
	}

	return -1;
}

int32 USequentialTurnOrderStrategy::GetFirstParticipantIndex_Implementation(
	const TArray<FTurnParticipantInfo>& Participants)
{
	for (int32 i = 0; i < Participants.Num(); i++)
	{
		if (Participants[i].IsActiveParticipant())
			return i;
	}
	return -1;
}

FString USequentialTurnOrderStrategy::GetStrategyName_Implementation()
{
	return TEXT("Sequential");
}
