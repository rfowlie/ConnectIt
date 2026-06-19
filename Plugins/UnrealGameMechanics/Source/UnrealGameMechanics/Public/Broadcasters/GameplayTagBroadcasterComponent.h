// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "GameplayTagBroadcasterComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameplayTagBroadcasterDelegate, FGameplayTag, Tag);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNREALGAMEMECHANICS_API UGameplayTagBroadcasterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGameplayTagBroadcasterComponent();

	UPROPERTY(BlueprintAssignable)
	FGameplayTagBroadcasterDelegate GameplayTagBroadcaster;

protected:
	UFUNCTION(BlueprintCallable)
	void Broadcast(FGameplayTag Tag);
	
};
