// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameTurnParticipantDelegateComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTurnParticipantDelegate);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNREALGAMEMECHANICS_API UGameTurnParticipantDelegateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGameTurnParticipantDelegateComponent();

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FTurnParticipantDelegate OnTurnBegin;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FTurnParticipantDelegate OnTurnComplete;
};
