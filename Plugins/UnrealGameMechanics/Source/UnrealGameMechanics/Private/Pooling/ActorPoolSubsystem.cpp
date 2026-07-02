// Fill out your copyright notice in the Description page of Project Settings.


#include "Pooling/ActorPoolSubsystem.h"
#include "Pooling/ActorPool.h"


TArray<AActor*> UActorPoolSubsystem::GetObjects(TSubclassOf<AActor> ActorClass, int32 Amount, int32 InitialPoolSize)
{
	if (!ActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("UActorPoolSubsystem::GetObjects - ActorClass nullptr"));
		return {};
	}

	UActorPool* Pool = FindPool(ActorClass);
	if (!Pool)
	{
		Pool = CreatePool(ActorClass, InitialPoolSize);
	}

	if (!Pool)
	{
		return {};
	}

	TArray<AActor*> Actors = Pool->GetObjects(Amount);

	// record where each actor came from so ReleaseObject can route without the caller knowing
	for (AActor* Actor : Actors)
	{
		ActorToPool.Add(Actor, Pool);
	}

	return Actors;
}

void UActorPoolSubsystem::ReleaseObject(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	if (const TObjectPtr<UActorPool>* Pool = ActorToPool.Find(Actor))
	{
		(*Pool)->ReleaseObject(Actor);
		ActorToPool.Remove(Actor);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("UActorPoolSubsystem::ReleaseObject - Actor %s was not sourced from any tracked pool"), *Actor->GetName());
}

UActorPool* UActorPoolSubsystem::FindPool(TSubclassOf<AActor> ActorClass) const
{
	if (const TObjectPtr<UActorPool>* Found = Pools.Find(ActorClass))
	{
		return *Found;
	}

	return nullptr;
}

UActorPool* UActorPoolSubsystem::CreatePool(TSubclassOf<AActor> ActorClass, int32 InitialPoolSize)
{
	if (!ActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("UActorPoolSubsystem::CreatePool - ActorClass nullptr"));
		return nullptr;
	}

	if (UActorPool* Existing = FindPool(ActorClass))
	{
		return Existing;
	}

	UActorPool* NewPool = UActorPool::Create(GetWorld(), ActorClass, InitialPoolSize);
	if (NewPool)
	{
		Pools.Add(ActorClass, NewPool);
	}

	return NewPool;
}

void UActorPoolSubsystem::Deinitialize()
{
	Pools.Empty();
	ActorToPool.Empty();

	Super::Deinitialize();
}