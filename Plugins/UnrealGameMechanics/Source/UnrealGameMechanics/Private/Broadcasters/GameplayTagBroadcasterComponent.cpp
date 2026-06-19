// Fill out your copyright notice in the Description page of Project Settings.


#include "Broadcasters/GameplayTagBroadcasterComponent.h"


// Sets default values for this component's properties
UGameplayTagBroadcasterComponent::UGameplayTagBroadcasterComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void UGameplayTagBroadcasterComponent::Broadcast(FGameplayTag Tag)
{
	if (GameplayTagBroadcaster.IsBound()) { GameplayTagBroadcaster.Broadcast(Tag); }
}
