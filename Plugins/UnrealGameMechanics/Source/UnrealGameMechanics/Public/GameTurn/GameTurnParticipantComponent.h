// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameTurnParticipantComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTurnParticipantDelegate);

/*
 * Ideally for game modes to give to controllers to then notify
 * various turn states
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNREALGAMEMECHANICS_API UGameTurnParticipantComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGameTurnParticipantComponent();

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FTurnParticipantDelegate OnTurnBegin;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FTurnParticipantDelegate OnTurnEnd;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FTurnParticipantDelegate OnTurnPause;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FTurnParticipantDelegate OnTurnResume;
};
