// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TurnBasedStrategyStructs.h"
#include "TurnOrderInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UTurnOrderInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UNREALTURNBASEDSTRATEGYMECHANICS_API ITurnOrderInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	// Returns the index of the next active participant
	// CurrentIndex is the index that just finished their turn
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Turn Based | Order")
	int32 GetNextParticipantIndex(
		const TArray<FTurnParticipantInfo>& Participants,
		int32 CurrentIndex);

	// Returns the index of the first participant to act
	// Called once at game start
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Turn Based | Order")
	int32 GetFirstParticipantIndex(
		const TArray<FTurnParticipantInfo>& Participants);

	// Display name for debugging and logging
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Turn Based | Order")
	FString GetStrategyName();
};
