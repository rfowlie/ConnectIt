// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameTurnParticipant.generated.h"


class UGameTurnParticipantDelegateComponent;

// This class does not need to be modified.
UINTERFACE(Blueprintable, BlueprintType)
class UGameTurnParticipant : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UNREALGAMEMECHANICS_API IGameTurnParticipant
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Game Mechanics | Turn")
	void TurnBegin();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Game Mechanics | Turn")
	void TurnComplete();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Game Mechanics | Turn")
	UGameTurnParticipantDelegateComponent* GetTurnParticipantDelegateComponent();
	
};
