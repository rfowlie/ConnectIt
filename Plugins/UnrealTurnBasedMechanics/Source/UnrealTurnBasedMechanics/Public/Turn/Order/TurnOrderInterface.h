// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TurnBasedMechanicsStructs.h"
#include "TurnOrderInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable, BlueprintType)
class UTurnOrderInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UNREALTURNBASEDMECHANICS_API ITurnOrderInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	// Returns index of next active participant
	// CurrentIndex is the participant that just finished
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable,
		Category = "Turn Based|Strategy")
	int32 GetNextParticipantIndex(
		const TArray<FTurnParticipantInfo>& Participants,
		int32 CurrentIndex);

	// Returns index of first participant to act
	// Called once at game start
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable,
		Category = "Turn Based|Strategy")
	int32 GetFirstParticipantIndex(
		const TArray<FTurnParticipantInfo>& Participants);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable,
		Category = "Turn Based|Strategy")
	FString GetStrategyName();
};
