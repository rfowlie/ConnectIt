// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "GameTurnParticipant.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameTurnParticipantDelegate, FGameplayTag, TurnTag);

/**
 * 
 */
UCLASS(BlueprintType)
class UNREALGAMEMECHANICS_API UGameTurnParticipant : public UObject
{
	GENERATED_BODY()

public:
	UGameTurnParticipant();

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FGameTurnParticipantDelegate OnTurnBegin;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FGameTurnParticipantDelegate OnTurnEnd;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FGameTurnParticipantDelegate OnTurnPause;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FGameTurnParticipantDelegate OnTurnResume;
};
