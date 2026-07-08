// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "TurnBasedMechanicsStructs.h"
#include "TurnBasedGameState.generated.h"

class UTurnBasedParticipantManagerComponent;

/**
 * 
 */
UCLASS()
class UNREALTURNBASEDMECHANICS_API ATurnBasedGameState : public AGameState
{
	GENERATED_BODY()

public:
	
	ATurnBasedGameState();
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Turn Based")
	UTurnBasedParticipantManagerComponent* ParticipantManager = nullptr;

	// Convenience accessors (for replicated properties)

	UFUNCTION(BlueprintPure, Category = "Turn Based")
	const TArray<FTurnParticipantInfo>& GetParticipants() const;

	UFUNCTION(BlueprintPure, Category = "Turn Based")
	int32 GetActiveTurnNumber() const;
	
	UFUNCTION(BlueprintPure, Category = "Turn Based")
	ETurnPhase GetCurrentPhase() const;

	UFUNCTION(BlueprintPure, Category = "Turn Based")
	float GetTurnDuration() const;
	
};
