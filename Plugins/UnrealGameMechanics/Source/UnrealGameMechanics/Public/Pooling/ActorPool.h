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

	// Retrieve Amount actors from the pool, growing the pool if needed. Does
	// NOT activate them -- the caller is responsible for calling
	// ActivateObject once it's ready (e.g. after binding any completion
	// listener the actor's own activation might synchronously fire), see
	// that function's comment.
	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	TArray<AActor*> GetObjects(int32 Amount = 1);

	// Return a previously retrieved actor to the pool. Does NOT deactivate
	// it -- call DeactivateObject explicitly, before or after this, per the
	// same caller-controls-timing reasoning as ActivateObject.
	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	void ReleaseObject(AActor* Actor);

	// Triggers Actor's pooled-activation side effect: IActorPoolInterface::
	// ActivatePoolObject if Actor implements it, else a default hide/
	// collision/tick toggle. Deliberately not called automatically by
	// GetObjects -- if Actor's activation can complete synchronously (a
	// documented, legitimate pattern for pieces with no real visual
	// effect), the caller needs to have already bound any completion
	// listener before calling this, or the completion signal is missed.
	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	void ActivateObject(AActor* Actor) const;

	// Mirror of ActivateObject for deactivation -- same caller-controls-
	// timing reasoning, see that function's comment.
	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	void DeactivateObject(AActor* Actor) const;

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

	void SetDefaultActivation(AActor* Actor, const bool bActivate) const;
};
