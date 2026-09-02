// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TurnBasedGameEvent.generated.h"

class UTurnBasedGameEvent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnBasedGameEventComplete, UTurnBasedGameEvent*, Event);

// Project-agnostic base for "something that happens as a reaction to a
// board/game-state change" -- a NewObject'd instance with its own
// lifecycle, not a persistent component, deliberately echoing
// UTurnBasedAction's shape (construct, kick off, self-report done) without
// any of that class's player-input/selection/cooldown machinery, none of
// which applies here.
//
// A concrete subclass owns its *entire* sequence -- e.g. retrieve a piece,
// initialize it, run its visual, wait for that visual to actually finish,
// then report completion -- as its own internal state (member fields),
// rather than that "am I still waiting" state living awkwardly on
// whatever dispatches these events. Construct with NewObject<T>(Outer, ...)
// where Outer is whatever owns this event's lifetime (typically the
// dispatching component) -- UObject::GetWorld()'s default implementation
// walks the Outer chain, so no manual override is needed here as long as
// construction always supplies a proper Outer.
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class UNREALTURNBASEDMECHANICS_API UTurnBasedGameEvent : public UObject
{
    GENERATED_BODY()

public:

    // Kicks off this event's own sequence. Subclasses override to do
    // whatever multistep work they need, calling Complete() once
    // genuinely done -- not merely started.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Turn Based|Game Event")
    void Execute();
    virtual void Execute_Implementation();

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Game Event")
    FOnTurnBasedGameEventComplete OnComplete;

protected:

    // Subclasses call this once their work is genuinely finished --
    // success or logged failure alike. Deliberately no separate Cancel/
    // error state -- these aren't player-cancellable, and a failure that
    // doesn't still call Complete() would stall whatever's waiting on this
    // event, the same class of bug every other gated-event error path in
    // this codebase already avoids by completing on failure too.
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Game Event")
    void Complete();
};
