// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/ConnectIt_VisualHandle.h"
#include "Engine/World.h"
#include "TimerManager.h"


void UConnectIt_VisualHandle::Hold(UObject* Holder)
{
    if (bComplete)
    {
        UE_LOG(LogTemp, Warning, TEXT("ConnectIt_VisualHandle: '%s' held %s after it completed -- ignored"),
            *GetNameSafe(Holder), *EventTag.ToString());
        return;
    }
    Holders.Add(Holder);
    HolderNames.Add(GetNameSafe(Holder));
}

void UConnectIt_VisualHandle::Release(UObject* Holder)
{
    if (bComplete)
    {
        UE_LOG(LogTemp, Warning, TEXT("ConnectIt_VisualHandle: '%s' released %s after it completed -- ignored"),
            *GetNameSafe(Holder), *EventTag.ToString());
        return;
    }

    const int32 Index = Holders.IndexOfByPredicate(
        [Holder](const TWeakObjectPtr<UObject>& Held) { return Held.Get() == Holder; });
    if (Index == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("ConnectIt_VisualHandle: '%s' released %s without holding it -- ignored"),
            *GetNameSafe(Holder), *EventTag.ToString());
        return;
    }
    Holders.RemoveAt(Index);
    HolderNames.RemoveAt(Index);
    TryComplete();
}

void UConnectIt_VisualHandle::ReleaseAllBy(const UObject* Holder)
{
    if (bComplete || !Holder) return;

    bool bReleased = false;
    for (int32 Index = Holders.Num() - 1; Index >= 0; Index--)
    {
        if (Holders[Index].Get() == Holder)
        {
            Holders.RemoveAt(Index);
            HolderNames.RemoveAt(Index);
            bReleased = true;
        }
    }
    if (bReleased)
    {
        TryComplete();
    }
}

void UConnectIt_VisualHandle::Begin(FGameplayTag InEventTag, float TimeoutSeconds)
{
    EventTag = InEventTag;
    UWorld* World = GetWorld();
    if (TimeoutSeconds > 0.f && World)
    {
        World->GetTimerManager().SetTimer(TimeoutHandle,
            FTimerDelegate::CreateUObject(this, &UConnectIt_VisualHandle::HandleTimeout), TimeoutSeconds, false);
    }
}

void UConnectIt_VisualHandle::Seal()
{
    bSealed = true;
    TryComplete();
}

void UConnectIt_VisualHandle::TryComplete()
{
    if (bSealed && !bComplete && Holders.IsEmpty())
    {
        Complete();
    }
}

void UConnectIt_VisualHandle::Complete()
{
    bComplete = true;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TimeoutHandle);
    }
    OnComplete.Broadcast(this);
    OnComplete.Clear();
}

void UConnectIt_VisualHandle::HandleTimeout()
{
    if (bComplete) return;

    UE_LOG(LogTemp, Error,
        TEXT("ConnectIt_VisualHandle: %s timed out with %d hold(s) never released (%s) -- completed anyway"),
        *EventTag.ToString(), Holders.Num(), *FString::Join(HolderNames, TEXT(", ")));
    Holders.Reset();
    HolderNames.Reset();
    Complete();
}
