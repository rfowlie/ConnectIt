// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "Engine/TimerHandle.h"
#include "ConnectIt_VisualHandle.generated.h"

class UConnectIt_VisualHandle;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnConnectItVisualHandleComplete, UConnectIt_VisualHandle*);


// One playing board event's visuals: everything that animates for it Holds
// the handle while it plays and Releases it when done. The event (and the
// board after it) waits until every hold is released -- or at once, if
// nothing held it. The board visualizer (AConnectIt_BoardVisualizer) makes one
// per event and hands it to its event functions and the actors' hooks.
//
// A hold that is never released is logged by name and the event completes
// anyway after the visualizer's timeout, so a forgotten Release can't freeze
// the game. A hold or release after completion is ignored (warned).
UCLASS(BlueprintType)
class CONNECTIT_API UConnectIt_VisualHandle : public UObject
{
    GENERATED_BODY()

public:

    // Something started animating for this event; the event waits for a
    // matching Release. Holder is who holds (self, from Blueprint) -- for the
    // timeout's log, and so an actor leaving play can let go of its holds.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Visuals", meta = (DefaultToSelf = "Holder", HidePin = "Holder"))
    void Hold(UObject* Holder);

    // Holder's animation is done (one Release per Hold)
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Visuals", meta = (DefaultToSelf = "Holder", HidePin = "Holder"))
    void Release(UObject* Holder);

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Visuals")
    bool IsComplete() const { return bComplete; }

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Visuals")
    int32 GetHoldCount() const { return Holders.Num(); }

    // The playing event's tag
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Visuals")
    FGameplayTag GetEventTag() const { return EventTag; }

    // Releases every hold Holder still has (an actor leaving play)
    void ReleaseAllBy(const UObject* Holder);

    // Completed by its timeout rather than by every hold being released
    bool HasTimedOut() const { return bTimedOut; }

    // Who still held it when it timed out (empty if it didn't)
    const FString& GetTimedOutHolders() const { return TimedOutHolders; }

    // --- The visualizer's ---

    // Starts the handle for an event; a timeout above 0 completes it (with a
    // log of what still holds) if the holds outlast it. Needs a world (the
    // handle's outer) for the timeout.
    void Begin(FGameplayTag InEventTag, float TimeoutSeconds);

    // Every event function has been called: the handle completes once no
    // hold is left -- now, if there is none
    void Seal();

    // Fires once, on completion
    FOnConnectItVisualHandleComplete OnComplete;

private:

    void TryComplete();
    void Complete();
    void HandleTimeout();

    // One entry per hold (a holder can hold more than once)
    TArray<TWeakObjectPtr<UObject>> Holders;
    TArray<FString> HolderNames;

    FGameplayTag EventTag;
    FTimerHandle TimeoutHandle;
    bool bSealed = false;
    bool bComplete = false;
    bool bTimedOut = false;
    FString TimedOutHolders;
};
