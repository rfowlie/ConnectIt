// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TurnBasedStrategyStructs.h"
#include "UObject/Interface.h"
#include "TurnOrderHandlerInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UTurnOrderHandlerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UNREALTURNBASEDSTRATEGYMECHANICS_API ITurnOrderHandlerInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	// virtual ~ITurnOrderHandlerInterface() = default;

	// Returns the index into Participants of the next active participant
	// CurrentIndex is the index that just finished their turn
	// Participants contains the full current state including forfeit/connect status
	virtual int32 GetNextParticipantIndex(
		const TArray<FTurnParticipantInfo>& Participants,
		int32 CurrentIndex) = 0;

	// Returns the index of the first participant to go
	// Called once at game start
	virtual int32 GetFirstParticipantIndex(
		const TArray<FTurnParticipantInfo>& Participants) = 0;

	// Display name for debugging and logging
	virtual FString GetStrategyName() const = 0;
};
