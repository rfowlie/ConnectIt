// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TurnOrderInterface.h"
#include "UObject/Object.h"
#include "SequentialTurnOrderStrategy.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class UNREALTURNBASEDMECHANICS_API USequentialTurnOrderStrategy : public UObject, public ITurnOrderInterface
{
	GENERATED_BODY()

public:

	virtual int32 GetNextParticipantIndex_Implementation(
		const TArray<FTurnParticipantInfo>& Participants,
		int32 CurrentIndex) override;

	virtual int32 GetFirstParticipantIndex_Implementation(
		const TArray<FTurnParticipantInfo>& Participants) override;

	virtual FString GetStrategyName_Implementation() override;
	
};
