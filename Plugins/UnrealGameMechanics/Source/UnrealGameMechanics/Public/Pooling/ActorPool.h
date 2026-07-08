#pragma once

#include "CoreMinimal.h"
#include "ActorPool.generated.h"

class UActorPoolInterface;

UCLASS(BlueprintType)
class UNREALGAMEMECHANICS_API UActorPool : public UObject
{
	GENERATED_BODY()

public:
	UActorPool();

	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	static UActorPool* Create(UWorld* InWorldContext, TSubclassOf<AActor> InActorClass, int32 InitialPoolSize = 1);

	// Retrieve Amount actors from the pool, activating them and growing the pool if needed
	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	TArray<AActor*> GetObjects(int32 Amount = 1);

	// Return a previously retrieved actor to the pool
	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	void ReleaseObject(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	TSubclassOf<AActor> GetActorClass() const { return ActorClass; }

	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	int32 GetActivePoolCount() const { return ActivePool.Num(); }

	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	int32 GetInactivePoolCount() const { return InactivePool.Num(); }

	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	int32 GetTotalPoolCount() const { return ActivePool.Num() + InactivePool.Num(); }

private:
	UPROPERTY()
	TObjectPtr<UWorld> WorldContext = nullptr;

	UPROPERTY()
	TSubclassOf<AActor> ActorClass;

	
	TSet<TObjectPtr<AActor>> ActivePool;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> InactivePool;

	AActor* CreatePooledActor() const;

	void ActivateActor(AActor* Actor) const;
	void DeactivateActor(AActor* Actor) const;	
	void SetDefaultActivation(AActor* Actor, const bool bActivate) const;
};
