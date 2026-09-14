// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ActorPoolSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class UNREALGAMEMECHANICS_API UActorPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Fetch Amount actors of ActorClass, creating a pool for that class on first use
	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	TArray<AActor*> GetObjects(TSubclassOf<AActor> ActorClass, int32 Amount = 1, int32 InitialPoolSize = 1);

	// Return an actor to its pool. Actor must have originated from GetObjects.
	// Does NOT deactivate it -- call DeactivateObject explicitly.
	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	void ReleaseObject(AActor* Actor);

	// Triggers Actor's pooled-activation/deactivation side effect via its
	// owning UActorPool (resolved the same way ReleaseObject is -- Actor
	// must have originated from GetObjects). See UActorPool::ActivateObject/
	// DeactivateObject's own comment for why this is a separate, explicit
	// call rather than something GetObjects/ReleaseObject do automatically.
	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	void ActivateObject(AActor* Actor) const;

	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	void DeactivateObject(AActor* Actor) const;

	// Returns the pool for a class if one has been created, otherwise nullptr
	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	UActorPool* FindPool(TSubclassOf<AActor> ActorClass) const;

	// Explicitly create/pre-warm a pool ahead of first use
	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	UActorPool* CreatePool(TSubclassOf<AActor> ActorClass, int32 InitialPoolSize = 1);

	virtual void Deinitialize() override;

private:
	UPROPERTY()
	TMap<TSubclassOf<AActor>, TObjectPtr<UActorPool>> Pools;

	// Reverse lookup so ReleaseObject doesn't need the caller to know which pool an actor came from
	UPROPERTY()
	TMap<TObjectPtr<AActor>, TObjectPtr<UActorPool>> ActorToPool;
};
