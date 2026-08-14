// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/ConnectIt_BoardSequencerComponent.h"

#include "Board/ConnectIt_BoardStateComponent.h"
#include "ConnectIt_GameplayTags.h"
#include "GameplayTagContainer.h"
#include "GameEvent/GameEventTaskSubsystem.h"


void UConnectIt_BoardSequencerComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!IsValid(GetOwner()))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardSequencerComponent: BeginPlay — no owner"));
        return;
    }

    BoardStateComponent = GetOwner()->FindComponentByClass<UConnectIt_BoardStateComponent>();

    if (!IsValid(BoardStateComponent))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardSequencerComponent: BeginPlay — "
                 "no UConnectIt_BoardStateComponent found on owner"));
        return;
    }

    BoardStateComponent->OnBoardStateChanged.AddDynamic(
        this, &UConnectIt_BoardSequencerComponent::HandleBoardStateChanged);
}

void UConnectIt_BoardSequencerComponent::HandleBoardStateChanged()
{
    if (!IsValid(BoardStateComponent)) return;

    const FConnectItBoardStateSnapshot* Snapshot = BoardStateComponent->GetBoardSnapshot();
    if (!Snapshot) return;

    const FConnectItBoardChangeEvent& Event = Snapshot->ChangeEvent;
    PendingChangeEventQueue.Add(Event);
    TryStartNextSequence();
}

void UConnectIt_BoardSequencerComponent::TryStartNextSequence()
{
    if (bSequenceInFlight) return;
    if (PendingChangeEventQueue.IsEmpty()) return;

    bSequenceInFlight = true;
    ActiveChangeEvent = PendingChangeEventQueue[0];
    PendingChangeEventQueue.RemoveAt(0);

    // Build the step list -- Shift and PiecePlaced never co-occur on the
    // same ChangeEvent (see FConnectItBoardChangeEvent), then conditionally
    // LineScored and/or PlayerWin, in that order. Empty steps are simply
    // omitted rather than included and skipped.
    TArray<FGameplayTagContainer> Steps;
    if (ActiveChangeEvent.bShiftApplied)
    {
        Steps.Add(FGameplayTagContainer(ConnectIt_Event_Shift));
    }
    if (ActiveChangeEvent.bPiecePlaced)
    {
        Steps.Add(FGameplayTagContainer(ConnectIt_Event_PiecePlaced));
    }
    if (ActiveChangeEvent.bLineScored)
    {
        Steps.Add(FGameplayTagContainer(ConnectIt_Event_LineScored));
    }
    if (ActiveChangeEvent.bGameWon)
    {
        Steps.Add(FGameplayTagContainer(ConnectIt_Event_PlayerWin));
    }

    // change event nothing to report, catch false reports
    if (Steps.IsEmpty())
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardSequencerComponent: TyrStartNextSequence — "
                 "no steps to register for event"));        
        return;
    }

    UGameEventTaskSubsystem* GameEventSubsystem =
        GetWorld() ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>() : nullptr;

    if (!IsValid(GameEventSubsystem))
    {
        // No subsystem to trigger tags on -- nothing left to notify (no
        // typed delegates exist anymore; listeners bind directly to the
        // subsystem), so just consider this sequence done and drain the
        // next queued change event, if any
        HandleSequenceComplete();
        return;
    }

    // No OnStepComplete handler needed -- there are no typed delegates left
    // for this component to fire per-step. "Tell me now" listeners bind
    // UGameEventTaskSubsystem::BindOnTagBegin directly (fires the instant
    // TriggerTag runs, before any registered task executes); anyone needing
    // payload data reads BoardStateComponent->GetChangeEvent().
    FOnTagSequenceComplete CompleteDelegate;
    CompleteDelegate.BindDynamic(this, &UConnectIt_BoardSequencerComponent::HandleSequenceComplete);

    GameEventSubsystem->QueueTagSequence(Steps, FOnTagSequenceStepComplete(), CompleteDelegate);
}

void UConnectIt_BoardSequencerComponent::HandleSequenceComplete()
{
    bSequenceInFlight = false;
    TryStartNextSequence();

    if (IsIdle())
    {
        OnSequenceIdle.Broadcast();
    }
}
