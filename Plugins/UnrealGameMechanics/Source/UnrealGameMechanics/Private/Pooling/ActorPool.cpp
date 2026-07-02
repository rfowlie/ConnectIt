// Fill out your copyright notice in the Description page of Project Settings.

#include "Pooling/ActorPool.h"
#include "Pooling/ActorPoolInterface.h"

UActorPool::UActorPool()
{
}

UActorPool* UActorPool::Create(UWorld* InWorldContext, const TSubclassOf<AActor> InActorClass, const int32 InitialPoolSize)
{
    if (!InWorldContext)
    {
        UE_LOG(LogTemp, Error, TEXT("UActorPool::Create - World Context nullptr"));
        return nullptr;
    }

    if (!InActorClass)
    {
        UE_LOG(LogTemp, Error, TEXT("UActorPool::Create - Actor class nullptr"));
        return nullptr;
    }

    UActorPool* Obj = NewObject<UActorPool>();
    Obj->WorldContext = InWorldContext;
    Obj->ActorClass = InActorClass;

    const int32 ClampedInitialSize = FMath::Max(0, InitialPoolSize);
    Obj->InactivePool.Reserve(ClampedInitialSize);

    for (int32 i = 0; i < ClampedInitialSize; i++)
    {
        if (AActor* Actor = Obj->CreatePooledActor())
        {
            Obj->InactivePool.Add(Actor);
        }
    }

    return Obj;
}

TArray<AActor*> UActorPool::GetObjects(int32 Amount)
{
    Amount = FMath::Max(1, Amount);
    if (InactivePool.Num() < Amount)
    {
        InactivePool.Reserve(ActivePool.Num() + (Amount - InactivePool.Num()));
    }

    TArray<AActor*> OutActors;
    OutActors.Reserve(Amount);

    // pull from the inactive pool first, back-to-front removal is O(1)
    while (InactivePool.Num() > 0 && OutActors.Num() < Amount)
    {
        AActor* Actor = InactivePool.Pop(EAllowShrinking::No);
        if (!IsValid(Actor))
        {
            continue;
        }

        ActivateActor(Actor);
        ActivePool.Add(Actor);
        OutActors.Add(Actor);
    }

    // not enough inactive actors remained, spawn the rest
    while (OutActors.Num() < Amount)
    {
        AActor* Actor = CreatePooledActor();
        if (!Actor)
        {
            break;
        }

        ActivateActor(Actor);
        ActivePool.Add(Actor);
        OutActors.Add(Actor);
    }

    return OutActors;
}

void UActorPool::ReleaseObject(AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return;
    }

    if (ActivePool.Remove(Actor) == 0)
    {
        // wasn't part of the active pool - ignore
        return;
    }

    DeactivateActor(Actor);
    InactivePool.Add(Actor);
}

AActor* UActorPool::CreatePooledActor() const
{
    if (!WorldContext || !ActorClass)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* Actor = WorldContext->SpawnActor(ActorClass, nullptr, nullptr, SpawnParams);
    if (Actor)
    {
        DeactivateActor(Actor);
    }

    return Actor;
}

void UActorPool::ActivateActor(AActor* Actor) const
{
    if (Actor->GetClass()->ImplementsInterface(UActorPoolInterface::StaticClass()))
    {
        IActorPoolInterface::Execute_ActivatePoolObject(Actor);
    }
    else
    {
        SetDefaultActivation(Actor, true);
    }
}

void UActorPool::DeactivateActor(AActor* Actor) const
{
    if (Actor->GetClass()->ImplementsInterface(UActorPoolInterface::StaticClass()))
    {
        IActorPoolInterface::Execute_DeactivatePoolObject(Actor);
    }
    else
    {
        SetDefaultActivation(Actor, false);
    }
}

void UActorPool::SetDefaultActivation(AActor* Actor, const bool bActivate) const
{
    if (!IsValid(Actor)) {  return; }
    Actor->SetActorHiddenInGame(!bActivate);
    Actor->SetActorEnableCollision(bActivate);
    Actor->SetActorTickEnabled(bActivate);
}
