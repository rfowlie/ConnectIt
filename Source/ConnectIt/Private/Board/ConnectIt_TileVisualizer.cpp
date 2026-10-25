// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/ConnectIt_TileVisualizer.h"
#include "Board/Actor/ConnectIt_TileBase.h"
#include "Board/ConnectIt_TileReaction.h"
#include "Board/ConnectIt_TileRegistry.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "ConnectIt_GameplayTags.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "GameEvent/GameEventTaskSubsystem.h"
#include "GameEvent/GameEventTask_Async.h"
#include "Engine/World.h"
#include "TimerManager.h"


TArray<FGameplayTag> UConnectIt_TileVisualizer::GetHandledEventTags()
{
    return {
        ConnectIt_Event_TilePowerTriggered,
        ConnectIt_Event_PiecePlaced,
        ConnectIt_Event_PiecesSwapped,
        ConnectIt_Event_PieceCaptured,
        ConnectIt_Event_PieceRemoved,
        ConnectIt_Event_PieceMoved,
        ConnectIt_Event_Scored,
        ConnectIt_Event_MultiplierChanged,
        ConnectIt_Event_TileMultiplierDestroyed,
        ConnectIt_Event_TileLockChanged,
        ConnectIt_Event_TileActiveToggled,
        ConnectIt_Event_ChainPartnerChanged,
    };
}

void UConnectIt_TileVisualizer::Initialise(UConnectIt_TileRegistry* InTileRegistry)
{
    TileRegistry = InTileRegistry;

    UGameEventTaskSubsystem* Queue = GetWorld() ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>() : nullptr;
    if (!Queue)
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectIt_TileVisualizer: no game event queue -- tile hooks won't be called"));
        return;
    }

    for (const FGameplayTag& Tag : GetHandledEventTags())
    {
        UGameEventTask_Async* Task = NewObject<UGameEventTask_Async>(this);
        Task->bIsPersistentTask = true;
        Task->OnExecuteDelegate.BindDynamic(this, &UConnectIt_TileVisualizer::HandleTaskExecute);
        Queue->RegisterAsyncTask(Tag, Task, 0);
        Tasks.Add(Tag, Task);
    }
}

void UConnectIt_TileVisualizer::HandleTaskExecute()
{
    const UGameEventTaskSubsystem* Queue = GetWorld() ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>() : nullptr;
    if (!Queue) return;

    const FInstancedStruct& Payload = Queue->GetActivePayload();
    const FConnectItBoardEvent* Event = Payload.GetPtr<FConnectItBoardEvent>();
    const TObjectPtr<UGameEventTask_Async>* Task = Event ? Tasks.Find(Event->GetEventTag()) : nullptr;
    if (!Task) return;

    ActiveTask = *Task;
    PendingReactions = 0;
    bDispatching = true;
    Dispatch(Payload);
    bDispatching = false;

    // Nothing to wait on (no tile actors, or every hook finished at once):
    // done -- next tick, while the queue isn't walking its task list
    if (PendingReactions == 0)
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateUObject(this, &UConnectIt_TileVisualizer::CompleteActiveTask));
    }
}

void UConnectIt_TileVisualizer::Dispatch(const FInstancedStruct& Payload)
{
    // The board as the player has seen it so far: before this event (pieces'
    // owners for a swap)
    const UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    const auto FactionAt = [BoardState](const FGridPosition& Position)
    {
        const FConnectItTileData* Tile = BoardState ? BoardState->GetDisplayBoard().GetTileData(Position) : nullptr;
        return Tile ? Tile->FactionPiece : INDEX_NONE;
    };

    if (const auto* Triggered = Payload.GetPtr<FConnectItBoardEvent_TilePowerTriggered>())
    {
        if (AConnectIt_TileBase* Tile = TileAt(Triggered->Position))
        {
            Tile->OnTilePowerTriggered(*Triggered, MakeReaction(Tile));
        }
    }
    else if (const auto* Placed = Payload.GetPtr<FConnectItBoardEvent_PiecePlaced>())
    {
        if (AConnectIt_TileBase* Tile = TileAt(Placed->Position))
        {
            Tile->OnPieceEnter(Placed->Faction, ConnectIt_Event_PiecePlaced, MakeReaction(Tile));
        }
    }
    else if (const auto* Swapped = Payload.GetPtr<FConnectItBoardEvent_PiecesSwapped>())
    {
        const int32 FactionA = FactionAt(Swapped->PositionA);
        const int32 FactionB = FactionAt(Swapped->PositionB);
        if (AConnectIt_TileBase* TileA = TileAt(Swapped->PositionA))
        {
            TileA->OnPieceExit(FactionA, ConnectIt_Event_PiecesSwapped, MakeReaction(TileA));
            TileA->OnPieceEnter(FactionB, ConnectIt_Event_PiecesSwapped, MakeReaction(TileA));
        }
        if (AConnectIt_TileBase* TileB = TileAt(Swapped->PositionB))
        {
            TileB->OnPieceExit(FactionB, ConnectIt_Event_PiecesSwapped, MakeReaction(TileB));
            TileB->OnPieceEnter(FactionA, ConnectIt_Event_PiecesSwapped, MakeReaction(TileB));
        }
    }
    else if (const auto* Captured = Payload.GetPtr<FConnectItBoardEvent_PieceCaptured>())
    {
        if (AConnectIt_TileBase* Tile = TileAt(Captured->Position))
        {
            Tile->OnPieceEnter(Captured->CapturingFaction, ConnectIt_Event_PieceCaptured, MakeReaction(Tile));
        }
    }
    else if (const auto* Removed = Payload.GetPtr<FConnectItBoardEvent_PieceRemoved>())
    {
        if (AConnectIt_TileBase* Tile = TileAt(Removed->Position))
        {
            Tile->OnPieceExit(Removed->RemovedFaction, ConnectIt_Event_PieceRemoved, MakeReaction(Tile));
        }
    }
    else if (const auto* Moved = Payload.GetPtr<FConnectItBoardEvent_PieceMoved>())
    {
        if (AConnectIt_TileBase* From = TileAt(Moved->From))
        {
            From->OnPieceExit(Moved->Faction, ConnectIt_Event_PieceMoved, MakeReaction(From));
        }
        if (AConnectIt_TileBase* To = TileAt(Moved->To))
        {
            To->OnPieceEnter(Moved->Faction, ConnectIt_Event_PieceMoved, MakeReaction(To));
        }
    }
    else if (const auto* Scored = Payload.GetPtr<FConnectItBoardEvent_Scored>())
    {
        for (int32 Index = 0; Index < Scored->Positions.Num() && Index < Scored->NewMultipliers.Num(); Index++)
        {
            if (AConnectIt_TileBase* Tile = TileAt(Scored->Positions[Index]))
            {
                Tile->OnMultiplierChanged(Scored->NewMultipliers[Index], MakeReaction(Tile));
            }
        }
    }
    else if (const auto* Adjusted = Payload.GetPtr<FConnectItBoardEvent_MultiplierChanged>())
    {
        if (AConnectIt_TileBase* Tile = TileAt(Adjusted->Position))
        {
            Tile->OnMultiplierChanged(Adjusted->NewMultiplier, MakeReaction(Tile));
        }
    }
    else if (const auto* Destroyed = Payload.GetPtr<FConnectItBoardEvent_TileMultiplierDestroyed>())
    {
        if (AConnectIt_TileBase* Tile = TileAt(Destroyed->Position))
        {
            Tile->OnMultiplierChanged(Destroyed->NewMultiplier, MakeReaction(Tile));
        }
    }
    else if (const auto* Lock = Payload.GetPtr<FConnectItBoardEvent_TileLockChanged>())
    {
        if (AConnectIt_TileBase* Tile = TileAt(Lock->Position))
        {
            Tile->OnLockChanged(Lock->bLocked, MakeReaction(Tile));
        }
    }
    else if (const auto* Toggled = Payload.GetPtr<FConnectItBoardEvent_TileActiveToggled>())
    {
        if (AConnectIt_TileBase* Tile = TileAt(Toggled->Position))
        {
            Tile->OnActiveChanged(Toggled->bNowActive, MakeReaction(Tile));
        }
    }
    else if (const auto* Partnered = Payload.GetPtr<FConnectItBoardEvent_ChainPartnerChanged>())
    {
        if (AConnectIt_TileBase* Chained = TileAt(Partnered->Position))
        {
            Chained->OnChainPartnerChanged(*Partnered, MakeReaction(Chained));
        }
        if (Partnered->bHasPartner)
        {
            if (AConnectIt_TileBase* Partner = TileAt(Partnered->PartnerPosition))
            {
                Partner->OnChainPartnerChanged(*Partnered, MakeReaction(Partner));
            }
        }
    }
}

UConnectIt_TileReaction* UConnectIt_TileVisualizer::MakeReaction(AConnectIt_TileBase* Tile)
{
    UConnectIt_TileReaction* Reaction = NewObject<UConnectIt_TileReaction>(this);
    PendingReactions++;
    TWeakObjectPtr<UConnectIt_TileVisualizer> WeakThis(this);
    Reaction->OnFinished = [WeakThis]()
    {
        if (UConnectIt_TileVisualizer* Visualizer = WeakThis.Get())
        {
            Visualizer->HandleReactionFinished();
        }
    };
    Tile->HoldReaction(Reaction);
    return Reaction;
}

AConnectIt_TileBase* UConnectIt_TileVisualizer::TileAt(const FGridPosition& Position) const
{
    return IsValid(TileRegistry) ? Cast<AConnectIt_TileBase>(TileRegistry->GetTileAtPosition(Position)) : nullptr;
}

void UConnectIt_TileVisualizer::HandleReactionFinished()
{
    // While dispatching, the count is checked once the hooks have all been called
    if (--PendingReactions > 0 || bDispatching) return;
    CompleteActiveTask();
}

void UConnectIt_TileVisualizer::CompleteActiveTask()
{
    if (UGameEventTask_Async* Task = ActiveTask)
    {
        ActiveTask = nullptr;
        Task->CallOnComplete();
    }
}
