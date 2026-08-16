// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ConnectIt_Structs.h"
#include "ConnectIt_BoardSequencerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FConnectItOnSequenceIdle);

// Drives the gated visual sequence for board changes -- ConnectIt_Event_Shift
// (shift requests) or ConnectIt_Event_PiecePlaced -> ConnectIt_Event_LineScored
// -> ConnectIt_Event_PlayerWin (placement requests), in strict order via
// UGameEventTaskSubsystem's QueueTagSequence, each step gated on the previous
// fully completing. Self-contained: resolves its sibling BoardStateComponent
// and binds to its OnBoardStateChanged itself in BeginPlay. Requires GetWorld()
// to resolve UGameEventTaskSubsystem (a UWorldSubsystem) -- that's what makes
// this an ActorComponent rather than a plain UObject; a raw UObject has no
// working GetWorld() unless it explicitly overrides one.
//
// This component knows nothing about turns or UTurnBasedParticipantManagerComponent
// -- turn-end is not a board concern. It only exposes IsIdle()/OnSequenceIdle,
// a board-domain fact; AConnectIt_BoardManager reads that from its own
// persistent ConnectIt_Event_TurnEnd task to decide whether it can complete
// immediately or has to wait for board-change visuals to finish first (see
// Workflows/GameEventSubsystem_Workflow.txt's "TURN SYSTEM NOW WAITS ON
// THIS" section).
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CONNECTIT_API UConnectIt_BoardSequencerComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Sequencing")
    bool IsSequenceInFlight() const { return bSequenceInFlight; }

    // True when there is nothing currently playing AND nothing queued --
    // the only state from which it's safe for an external system to assume
    // no more board-change visuals are about to start on their own. This
    // component has no opinion on who queries it or why (e.g. AConnectIt_
    // BoardManager, orchestrating turn-end) -- purely a board-domain fact.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Sequencing")
    bool IsIdle() const { return !bSequenceInFlight && PendingChangeEventQueue.IsEmpty(); }

    // Fires whenever the sequence transitions into the idle state above.
    UPROPERTY(BlueprintAssignable, Category = "ConnectIt|Sequencing")
    FConnectItOnSequenceIdle OnSequenceIdle;

protected:

    virtual void BeginPlay() override;

private:

    // Bound to BoardStateComponentRef->OnBoardStateChanged in BeginPlay, on
    // both server and client -- that signal already fires symmetrically on
    // both machines (server: SetBoardState, client: OnRep_BoardSnapshot), so
    // reading BoardSnapshot.ChangeEvent here is what drives the gated visual
    // sequence identically on every machine, instead of the old pattern of
    // broadcasting delegates directly from server-only request handlers.
    UFUNCTION()
    void HandleBoardStateChanged();

    // Starts the next queued change event's sequence if one isn't already
    // running -- pops PendingChangeEventQueue into ActiveChangeEvent, builds
    // the step list from its flags, and calls QueueTagSequence once. No
    // typed delegates are fired from here or anywhere in this sequencing --
    // listeners bind directly to UGameEventTaskSubsystem (BindOnTagBegin for
    // "tell me now", a registered UGameEventTask_Async for "wait for my
    // visual to finish") and read UConnectIt_BoardStateComponent::
    // GetChangeEvent() for payload data.
    void TryStartNextSequence();

    // Bound as QueueTagSequence's OnComplete -- the whole sequence for
    // ActiveChangeEvent finished, drain the next queued change event
    UFUNCTION()
    void HandleSequenceComplete();

    // Resolved once in BeginPlay via GetOwner() -- this component only ever
    // sits on AConnectIt_BoardManager.
    UPROPERTY()
    TObjectPtr<class UConnectIt_BoardStateComponent> BoardStateComponent = nullptr;

    // Change events awaiting sequencing -- a new event arriving while a
    // sequence is already in flight is queued rather than dropped or
    // interleaved with the one currently running. This is separate from
    // (and in addition to) UGameEventTaskSubsystem's own internal queue --
    // that queue only serializes WHICH sequence runs when; this one is what
    // lets HandleSequenceComplete know a new ChangeEvent is waiting once the
    // current one finishes.
    UPROPERTY()
    TArray<FConnectItBoardChangeEvent> PendingChangeEventQueue;

    // The change event currently being sequenced -- valid only while
    // bSequenceInFlight is true
    UPROPERTY()
    FConnectItBoardChangeEvent ActiveChangeEvent;

    UPROPERTY()
    bool bSequenceInFlight = false;
};
