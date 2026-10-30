// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/ConnectIt_BoardVisualizer.h"
#include "Board/ConnectIt_VisualHandle.h"
#include "Board/ConnectIt_EventVisual.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Board/ConnectIt_PieceRegistry.h"
#include "Board/ConnectIt_TileRegistry.h"
#include "Board/Actor/ConnectIt_PieceBase.h"
#include "Board/Actor/ConnectIt_TileBase.h"
#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Framework/Data/ConnectIt_WorldSettings.h"
#include "Framework/Library/ConnectIt_BoardEventLibrary.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "Framework/Subsystem/ConnectIt_BoardRegistrySubsystem.h"
#include "GameEvent/GameEventTaskSubsystem.h"
#include "GameEvent/GameEventTask_Async.h"
#include "Registry/GridDefinition.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/UObjectIterator.h"
#include "HAL/IConsoleManager.h"

namespace
{
    TAutoConsoleVariable<int32> CVarVisualizerReconcile(
        TEXT("ConnectIt.Visualizer.Reconcile"), 2,
        TEXT("Check the board's actors against the registries after board events play: ")
        TEXT("0 off, 1 log mismatches, 2 log and put them right (default)"));
}


AConnectIt_BoardVisualizer::AConnectIt_BoardVisualizer()
{
    PrimaryActorTick.bCanEverTick = false;
    // Local to each machine: every machine plays its own visuals
    bReplicates = false;
}

AConnectIt_BoardVisualizer* AConnectIt_BoardVisualizer::GetBoardVisualizer(const UObject* WorldContextObject)
{
    const UWorld* World = IsValid(WorldContextObject) ? WorldContextObject->GetWorld() : nullptr;
    const UConnectIt_BoardRegistrySubsystem* Subsystem =
        World ? World->GetSubsystem<UConnectIt_BoardRegistrySubsystem>() : nullptr;
    return Subsystem ? Subsystem->GetBoardVisualizer() : nullptr;
}

const TArray<const UScriptStruct*>& AConnectIt_BoardVisualizer::GetAllEventStructs()
{
    // Built on first use; Blueprint structs can't derive from a C++ struct,
    // so the set never changes at runtime
    static const TArray<const UScriptStruct*> Structs = []()
    {
        TArray<const UScriptStruct*> Found;
        const UScriptStruct* Base = FConnectItBoardEvent::StaticStruct();
        const UScriptStruct* OperationBase = FConnectItBoardOperationEvent::StaticStruct();
        const UScriptStruct* ResultBase = FConnectItBoardResultEvent::StaticStruct();
        for (TObjectIterator<UScriptStruct> It; It; ++It)
        {
            if (*It == Base || *It == OperationBase || *It == ResultBase || !It->IsChildOf(Base)) continue;
            Found.Add(*It);
        }
        return Found;
    }();
    return Structs;
}

void AConnectIt_BoardVisualizer::Initialise()
{
    UGameEventTaskSubsystem* Queue = GetWorld() ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>() : nullptr;
    if (!Queue)
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectIt_BoardVisualizer: no game event queue -- board events won't play"));
        return;
    }

    for (const UScriptStruct* Struct : GetAllEventStructs())
    {
        FInstancedStruct Instance;
        Instance.InitializeAs(Struct);
        const FGameplayTag Tag = Instance.Get<FConnectItBoardEvent>().GetEventTag();
        if (!Tag.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("ConnectIt_BoardVisualizer: board event %s has no tag -- it is never played"),
                *Struct->GetName());
            continue;
        }
        if (Tasks.Contains(Tag)) continue;

        UGameEventTask_Async* Task = NewObject<UGameEventTask_Async>(this);
        Task->bIsPersistentTask = true;
        Task->OnExecuteDelegate.BindDynamic(this, &AConnectIt_BoardVisualizer::HandleTaskExecute);
        Queue->RegisterAsyncTask(Tag, Task, 0);
        Tasks.Add(Tag, Task);
    }

    if (const UConnectIt_LevelConfigDataAsset* LevelConfig = AConnectIt_WorldSettings::FindLevelConfig(this))
    {
        SetEventVisuals(LevelConfig->EventVisuals);
    }
}

// --- Event visuals ---

TMap<FGameplayTag, TSubclassOf<AConnectIt_EventVisual>> AConnectIt_BoardVisualizer::BuildEventVisualMap(
    const TArray<TSubclassOf<AConnectIt_EventVisual>>& Classes, TArray<FString>* OutErrors, TArray<FString>* OutWarnings)
{
    TMap<FGameplayTag, TSubclassOf<AConnectIt_EventVisual>> Map;
    const auto Warn = [OutWarnings](const FString& Message) { if (OutWarnings) OutWarnings->Add(Message); };

    for (int32 Index = 0; Index < Classes.Num(); Index++)
    {
        const TSubclassOf<AConnectIt_EventVisual>& Class = Classes[Index];
        if (!Class)
        {
            Warn(FString::Printf(TEXT("Event Visuals entry %d is empty"), Index));
            continue;
        }
        if (Class->HasAnyClassFlags(CLASS_Abstract))
        {
            Warn(FString::Printf(TEXT("Event visual %s is abstract -- make a Blueprint of it and list that"), *Class->GetName()));
            continue;
        }
        const FGameplayTag Tag = AConnectIt_EventVisual::GetEventTagOf(Class);
        if (!Tag.IsValid())
        {
            Warn(FString::Printf(TEXT("Event visual %s isn't for a board event -- derive it from an event kind's class "
                "(e.g. ConnectIt_EventVisual_PiecePlaced)"), *Class->GetName()));
            continue;
        }
        if (const TSubclassOf<AConnectIt_EventVisual>* First = Map.Find(Tag))
        {
            if (OutErrors)
            {
                OutErrors->Add(FString::Printf(TEXT("Event visuals %s and %s are both for %s -- one per event (the first is used)"),
                    *(*First)->GetName(), *Class->GetName(), *Tag.ToString()));
            }
            continue;
        }
        Map.Add(Tag, Class);
    }
    return Map;
}

void AConnectIt_BoardVisualizer::SetEventVisuals(const TArray<TSubclassOf<AConnectIt_EventVisual>>& Classes)
{
    TArray<FString> Problems;
    EventVisualClasses = BuildEventVisualMap(Classes, &Problems, &Problems);
    for (const FString& Problem : Problems)
    {
        UE_LOG(LogTemp, Warning, TEXT("ConnectIt_BoardVisualizer: %s"), *Problem);
    }

    for (const TPair<FGameplayTag, TObjectPtr<AConnectIt_EventVisual>>& Spawned : EventVisuals)
    {
        if (IsValid(Spawned.Value)) Spawned.Value->Destroy();
    }
    EventVisuals.Reset();
}

bool AConnectIt_BoardVisualizer::PlayEventVisual(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle)
{
    const FConnectItBoardEvent* Event = Payload.GetPtr<FConnectItBoardEvent>();
    const FGameplayTag Tag = Event ? Event->GetEventTag() : FGameplayTag();
    const TSubclassOf<AConnectIt_EventVisual>* Class = EventVisualClasses.Find(Tag);
    if (!Class || !GetWorld()) return false;

    TObjectPtr<AConnectIt_EventVisual>& Visual = EventVisuals.FindOrAdd(Tag);
    if (!IsValid(Visual))
    {
        FActorSpawnParameters Params;
        Params.Owner = this;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Params.ObjectFlags |= RF_Transient;
        Visual = GetWorld()->SpawnActor<AConnectIt_EventVisual>(*Class, GetActorTransform(), Params);
    }
    if (!IsValid(Visual)) return false;

    Visual->PlayEvent(this, Payload, Handle);
    return true;
}

void AConnectIt_BoardVisualizer::PlayDefaultVisuals(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle)
{
    Dispatch(Payload, Handle);
}

void AConnectIt_BoardVisualizer::ReleaseHoldsBy(const UObject* Holder)
{
    if (IsValid(ActiveHandle))
    {
        ActiveHandle->ReleaseAllBy(Holder);
    }
}

// --- Playing an event ---

void AConnectIt_BoardVisualizer::HandleTaskExecute()
{
    const UGameEventTaskSubsystem* Queue = GetWorld() ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>() : nullptr;
    if (!Queue) return;

    const FInstancedStruct& Payload = Queue->GetActivePayload();
    const FConnectItBoardEvent* Event = Payload.GetPtr<FConnectItBoardEvent>();
    const TObjectPtr<UGameEventTask_Async>* Task = Event ? Tasks.Find(Event->GetEventTag()) : nullptr;
    if (!Task) return;

    if (IsValid(ActiveHandle) && !ActiveHandle->IsComplete())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardVisualizer: %s started while %s was still playing -- the previous one is dropped"),
            *Event->GetEventTag().ToString(), *ActiveHandle->GetEventTag().ToString());
        FinishBookkeeping();
    }

    TGuardValue<bool> Executing(bExecuting, true);
    ActiveTask = *Task;
    ActivePayload = Payload;
    BindToBoardState();

    UConnectIt_VisualHandle* Handle = NewObject<UConnectIt_VisualHandle>(this);
    ActiveHandle = Handle;
    Handle->OnComplete.AddUObject(this, &AConnectIt_BoardVisualizer::HandleVisualsComplete);
    Handle->Begin(Event->GetEventTag(), HoldTimeoutSeconds);

    BeginBookkeeping(Payload);
    if (!PlayEventVisual(Payload, Handle))
    {
        Dispatch(Payload, Handle);
    }
    Handle->Seal();
}

void AConnectIt_BoardVisualizer::HandleVisualsComplete(UConnectIt_VisualHandle* Handle)
{
    if (Handle != ActiveHandle) return;

    FinishBookkeeping();
    ActiveHandle = nullptr;

    // Completed while the queue is still walking its task list: next tick
    if (bExecuting)
    {
        GetWorldTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateUObject(this, &AConnectIt_BoardVisualizer::CompleteActiveTask));
        return;
    }
    CompleteActiveTask();
}

void AConnectIt_BoardVisualizer::CompleteActiveTask()
{
    if (UGameEventTask_Async* Task = ActiveTask)
    {
        ActiveTask = nullptr;
        Task->CallOnComplete();
    }
}

void AConnectIt_BoardVisualizer::Dispatch(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle)
{
    if (const auto* Seeded = Payload.GetPtr<FConnectItBoardEvent_BoardSeeded>())            OnBoardSeeded(*Seeded, Handle);
    else if (const auto* Placed = Payload.GetPtr<FConnectItBoardEvent_PiecePlaced>())        OnPiecePlaced(*Placed, Handle);
    else if (const auto* Swapped = Payload.GetPtr<FConnectItBoardEvent_PiecesSwapped>())     OnPiecesSwapped(*Swapped, Handle);
    else if (const auto* Captured = Payload.GetPtr<FConnectItBoardEvent_PieceCaptured>())    OnPieceCaptured(*Captured, Handle);
    else if (const auto* Removed = Payload.GetPtr<FConnectItBoardEvent_PieceRemoved>())      OnPieceRemoved(*Removed, Handle);
    else if (const auto* Moved = Payload.GetPtr<FConnectItBoardEvent_PieceMoved>())          OnPieceMoved(*Moved, Handle);
    else if (const auto* Shifted = Payload.GetPtr<FConnectItBoardEvent_BoardShifted>())      OnBoardShifted(*Shifted, Handle);
    else if (const auto* Scored = Payload.GetPtr<FConnectItBoardEvent_Scored>())             OnScored(*Scored, Handle);
    else if (const auto* Adjusted = Payload.GetPtr<FConnectItBoardEvent_MultiplierChanged>()) OnMultiplierChanged(*Adjusted, Handle);
    else if (const auto* Destroyed = Payload.GetPtr<FConnectItBoardEvent_TileMultiplierDestroyed>()) OnTileMultiplierDestroyed(*Destroyed, Handle);
    else if (const auto* Lock = Payload.GetPtr<FConnectItBoardEvent_TileLockChanged>())      OnTileLockChanged(*Lock, Handle);
    else if (const auto* Toggled = Payload.GetPtr<FConnectItBoardEvent_TileActiveToggled>()) OnTileActiveToggled(*Toggled, Handle);
    else if (const auto* Partnered = Payload.GetPtr<FConnectItBoardEvent_ChainPartnerChanged>()) OnChainPartnerChanged(*Partnered, Handle);
    else if (const auto* Triggered = Payload.GetPtr<FConnectItBoardEvent_TilePowerTriggered>()) OnTilePowerTriggered(*Triggered, Handle);
    else if (const auto* Added = Payload.GetPtr<FConnectItBoardEvent_TilesAdded>())          OnTilesAdded(*Added, Handle);
    else if (const auto* TilesGone = Payload.GetPtr<FConnectItBoardEvent_TilesRemoved>())    OnTilesRemoved(*TilesGone, Handle);
    else if (const auto* Layout = Payload.GetPtr<FConnectItBoardEvent_LayoutChanged>())      OnLayoutChanged(*Layout, Handle);
    else if (const auto* Phase = Payload.GetPtr<FConnectItBoardEvent_PhaseChanged>())        OnPhaseChanged(*Phase, Handle);
    else if (const auto* Goals = Payload.GetPtr<FConnectItBoardEvent_GoalsChanged>())        OnGoalsChanged(*Goals, Handle);
    else if (const auto* Checkpoint = Payload.GetPtr<FConnectItBoardEvent_WinCheckpointReached>()) OnWinCheckpointReached(*Checkpoint, Handle);
    else if (const auto* Lost = Payload.GetPtr<FConnectItBoardEvent_LoseConditionMet>())     OnLoseConditionMet(*Lost, Handle);
    else if (const auto* Won = Payload.GetPtr<FConnectItBoardEvent_GameWon>())               OnGameWon(*Won, Handle);
    else OnBoardEvent(Payload, Handle);
}

// --- Bookkeeping (runs whatever the event functions do) ---

void AConnectIt_BoardVisualizer::BeginBookkeeping(const FInstancedStruct& Payload)
{
    ArrivingPieces.Reset();
    LeavingPieces.Reset();
    LeavingTiles.Reset();
    TilesToSnap.Reset();
    PiecesToSnap.Reset();

    UConnectIt_TileRegistry* Tiles = GetTileRegistry();
    UConnectIt_PieceRegistry* Pieces = GetPieceRegistry();
    const FConnectItBoardState* Display = GetDisplayBoard();

    // Moved pieces end on their tile (the registry already set their position)
    const auto Repositioned = [this](const FGridPosition& Position)
    {
        PiecesToSnap.Add(Position);
    };

    if (Payload.GetPtr<FConnectItBoardEvent_BoardSeeded>())
    {
        // The starting pieces (the board before the seed's own events)
        for (int32 Index = 0; Display && Index < Display->NumTiles(); Index++)
        {
            const FConnectItTileData& Tile = Display->GetTileDataAt(Index);
            const FGridPosition Position = Display->GetPositionAt(Index);
            if (Tile.bIsOccupied && !(Pieces && Pieces->GetPieceAtPosition(Position)))
            {
                if (AConnectIt_PieceBase* Piece = SpawnPieceAt(Position, Tile.FactionPiece))
                {
                    ArrivingPieces.Add(Piece);
                }
            }
        }
    }
    else if (const auto* Placed = Payload.GetPtr<FConnectItBoardEvent_PiecePlaced>())
    {
        if (AConnectIt_PieceBase* Piece = SpawnPieceAt(Placed->Position, Placed->Faction))
        {
            ArrivingPieces.Add(Piece);
        }
    }
    else if (const auto* Swapped = Payload.GetPtr<FConnectItBoardEvent_PiecesSwapped>())
    {
        if (Pieces) Pieces->SwapPieceActors(Swapped->PositionA, Swapped->PositionB);
        Repositioned(Swapped->PositionA);
        Repositioned(Swapped->PositionB);
    }
    else if (const auto* Removed = Payload.GetPtr<FConnectItBoardEvent_PieceRemoved>())
    {
        TakeLeavingPiece(Removed->Position);
    }
    else if (const auto* Moved = Payload.GetPtr<FConnectItBoardEvent_PieceMoved>())
    {
        if (Pieces) Pieces->MovePieceActor(Moved->From, Moved->To);
        Repositioned(Moved->To);
    }
    else if (const auto* Shifted = Payload.GetPtr<FConnectItBoardEvent_BoardShifted>())
    {
        // Whole tiles move, pieces with them
        if (Tiles) Tiles->ShiftTiles(Shifted->StartPositions, Shifted->EndPositions);
        if (Pieces) Pieces->ShiftPieceActors(Shifted->StartPositions, Shifted->EndPositions);
        for (const FGridPosition& End : Shifted->EndPositions)
        {
            TilesToSnap.Add(End);
            Repositioned(End);
        }
    }
    else if (const auto* Scored = Payload.GetPtr<FConnectItBoardEvent_Scored>())
    {
        for (const FGridPosition& Cleared : Scored->ClearedPositions)
        {
            TakeLeavingPiece(Cleared);
        }
    }
    else if (const auto* TilesGone = Payload.GetPtr<FConnectItBoardEvent_TilesRemoved>())
    {
        for (const FGridPosition& Position : TilesGone->Positions)
        {
            TakeLeavingPiece(Position);
        }
        if (Tiles)
        {
            for (AGridTileBase* Tile : Tiles->UnregisterTilesAt(TilesGone->Positions))
            {
                if (IsValid(Tile)) LeavingTiles.Add(Tile);
            }
        }
    }
    else if (const auto* Added = Payload.GetPtr<FConnectItBoardEvent_TilesAdded>())
    {
        if (Tiles) Tiles->SpawnLayoutTiles(Added->Cells);

        // Starting pieces: what the board holds there once the tiles are in
        FConnectItBoardState After = Display ? *Display : FConnectItBoardState();
        Added->ApplyTo(After);
        for (const FConnectItBoardLayoutCell& Cell : Added->Cells)
        {
            const FConnectItTileData* Tile = After.GetTileData(Cell.Position);
            if (Tile && Tile->bIsOccupied)
            {
                if (AConnectIt_PieceBase* Piece = SpawnPieceAt(Cell.Position, Tile->FactionPiece))
                {
                    ArrivingPieces.Add(Piece);
                }
            }
        }
    }
}

void AConnectIt_BoardVisualizer::FinishBookkeeping()
{
    UConnectIt_PieceRegistry* Pieces = GetPieceRegistry();
    for (const TPair<FGridPosition, TObjectPtr<AGridPieceBase>>& Leaving : LeavingPieces)
    {
        if (Pieces && IsValid(Leaving.Value))
        {
            Pieces->DeactivatePiece(Leaving.Value);
            Pieces->ReleasePiece(Leaving.Value);
        }
    }

    for (AGridTileBase* Tile : LeavingTiles)
    {
        if (IsValid(Tile)) Tile->Destroy();
    }

    // Whatever the visuals did, moved actors end on their cells: tiles first
    // (pieces rest on them)
    const UConnectIt_TileRegistry* Tiles = GetTileRegistry();
    const UGridDefinition* Grid = Tiles ? Tiles->GridDefinition.Get() : nullptr;
    for (const FGridPosition& Position : TilesToSnap)
    {
        AGridTileBase* Tile = Tiles ? Tiles->GetTileAtPosition(Position) : nullptr;
        if (IsValid(Tile) && Grid)
        {
            FVector Location = Grid->GridPositionToWorld(Position);
            Location.Z = Tile->GetActorLocation().Z;
            Tile->SetActorLocation(Location);
        }
    }
    for (const FGridPosition& Position : PiecesToSnap)
    {
        if (AConnectIt_PieceBase* Piece = GetPieceAt(Position))
        {
            Piece->SetActorLocation(GetPieceLocation(Position));
        }
    }

    // Then the actors the event named, against the registries
    if (IsValid(ActiveHandle))
    {
        const FString Tag = ActiveHandle->GetEventTag().ToString();
        LastEventNote = ActiveHandle->HasTimedOut()
            ? FString::Printf(TEXT("after %s, which timed out held by %s"), *Tag, *ActiveHandle->GetTimedOutHolders())
            : FString::Printf(TEXT("after %s"), *Tag);
    }
    TArray<FGridPosition> Touched = TilesToSnap;
    Touched.Append(PiecesToSnap);
    for (const AConnectIt_PieceBase* Piece : GetArrivingPieces())
    {
        Touched.Add(Piece->GetGridPosition());
    }
    if (const UScriptStruct* Struct = ActivePayload.GetScriptStruct())
    {
        CollectPositions(Struct, ActivePayload.GetMemory(), Touched);
    }
    ReconcileActors(Touched);

    ActivePayload.Reset();
    ArrivingPieces.Reset();
    LeavingPieces.Reset();
    LeavingTiles.Reset();
    TilesToSnap.Reset();
    PiecesToSnap.Reset();
}

// --- Reconcile ---

void AConnectIt_BoardVisualizer::BindToBoardState()
{
    if (bBoundToBoardState) return;

    if (UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this))
    {
        BoardState->OnBoardEventsPlayed.AddUniqueDynamic(this, &AConnectIt_BoardVisualizer::HandleBoardEventsPlayed);
        bBoundToBoardState = true;
    }
}

void AConnectIt_BoardVisualizer::HandleBoardEventsPlayed()
{
    ReconcileAll();
}

void AConnectIt_BoardVisualizer::CollectPositions(const UStruct* Struct, const void* Data, TArray<FGridPosition>& OutPositions)
{
    const UScriptStruct* PositionStruct = FGridPosition::StaticStruct();
    const auto Visit = [&OutPositions, PositionStruct](const UScriptStruct* ValueStruct, const void* Value)
    {
        if (ValueStruct == PositionStruct)
        {
            OutPositions.Add(*static_cast<const FGridPosition*>(Value));
        }
        else
        {
            CollectPositions(ValueStruct, Value, OutPositions);
        }
    };

    for (TFieldIterator<FProperty> It(Struct); It; ++It)
    {
        const void* Value = It->ContainerPtrToValuePtr<void>(Data);
        if (const FStructProperty* StructProperty = CastField<FStructProperty>(*It))
        {
            Visit(StructProperty->Struct, Value);
        }
        else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(*It))
        {
            const FStructProperty* Inner = CastField<FStructProperty>(ArrayProperty->Inner);
            if (!Inner) continue;

            FScriptArrayHelper Array(ArrayProperty, Value);
            for (int32 Index = 0; Index < Array.Num(); Index++)
            {
                Visit(Inner->Struct, Array.GetRawPtr(Index));
            }
        }
    }
}

void AConnectIt_BoardVisualizer::ReportMismatch(const AActor* Actor, const FGridPosition& Position, const FString& What, bool bFixed) const
{
    const FString After = LastEventNote.IsEmpty() ? FString() : FString::Printf(TEXT(" (%s)"), *LastEventNote);
    UE_LOG(LogTemp, Error, TEXT("ConnectIt_BoardVisualizer: reconcile -- %s at (%d, %d) %s%s%s"),
        *GetNameSafe(Actor), Position.X, Position.Y, *What, *After, bFixed ? TEXT(" -- put right") : TEXT(""));
}

int32 AConnectIt_BoardVisualizer::ReconcileActors(const TArray<FGridPosition>& Positions)
{
    const int32 Mode = CVarVisualizerReconcile.GetValueOnGameThread();
    UConnectIt_TileRegistry* Tiles = GetTileRegistry();
    UConnectIt_PieceRegistry* Pieces = GetPieceRegistry();
    const UGridDefinition* Grid = Tiles ? Tiles->GridDefinition.Get() : nullptr;
    if (Mode <= 0 || !Tiles) return 0;
    const bool bFix = Mode >= 2;

    TArray<FGridPosition> Unique;
    for (const FGridPosition& Position : Positions)
    {
        Unique.AddUnique(Position);
    }

    int32 Mismatches = 0;

    // Tiles first: pieces rest on them
    for (const FGridPosition& Position : Unique)
    {
        AGridTileBase* Tile = Tiles->GetTileAtPosition(Position);
        if (!IsValid(Tile) || !Grid) continue;

        const FVector Current = Tile->GetActorLocation();
        const FVector Cell = Grid->GridPositionToWorld(Position);
        const FVector Expected(Cell.X, Cell.Y, Current.Z);
        if (!Current.Equals(Expected, ReconcileTolerance))
        {
            Mismatches++;
            ReportMismatch(Tile, Position, FString::Printf(TEXT("is at %s, off its cell %s"),
                *Current.ToCompactString(), *Expected.ToCompactString()), bFix);
            if (bFix) Tile->SetActorLocation(Expected);
        }
    }

    for (const FGridPosition& Position : Unique)
    {
        AGridPieceBase* Piece = Pieces ? Pieces->GetPieceAtPosition(Position) : nullptr;
        if (!IsValid(Piece)) continue;

        const FGridPosition Cached = Piece->GetGridPosition();
        if (!(Cached == Position))
        {
            Mismatches++;
            ReportMismatch(Piece, Position, FString::Printf(TEXT("has its position as (%d, %d), not the registry's"),
                Cached.X, Cached.Y), bFix);
            if (bFix) Pieces->MapPiece(Position, Piece);
        }
        if (!Tiles->GetTileAtPosition(Position))
        {
            Mismatches++;
            ReportMismatch(Piece, Position, TEXT("has no tile under it (not put right here)"), false);
        }

        const FVector Current = Piece->GetActorLocation();
        const FVector Expected = GetPieceLocation(Position);
        if (!Current.Equals(Expected, ReconcileTolerance))
        {
            Mismatches++;
            ReportMismatch(Piece, Position, FString::Printf(TEXT("is at %s, not on its tile at %s"),
                *Current.ToCompactString(), *Expected.ToCompactString()), bFix);
            if (bFix) Piece->SetActorLocation(Expected);
        }
    }
    return Mismatches;
}

int32 AConnectIt_BoardVisualizer::ReconcileAll()
{
    if (CVarVisualizerReconcile.GetValueOnGameThread() <= 0) return 0;

    UConnectIt_TileRegistry* Tiles = GetTileRegistry();
    UConnectIt_PieceRegistry* Pieces = GetPieceRegistry();
    TArray<FGridPosition> Positions = Tiles ? Tiles->GetAllTilePositions() : TArray<FGridPosition>();
    if (Pieces) Positions.Append(Pieces->GetAllPiecePositions());
    int32 Mismatches = ReconcileActors(Positions);

    // The board against the piece registry (the display board has caught up
    // once a change has played out)
    const FConnectItBoardState* Board = GetDisplayBoard();
    if (Board && Pieces && Board->NumTiles() > 0)
    {
        for (int32 Index = 0; Index < Board->NumTiles(); Index++)
        {
            const FGridPosition Position = Board->GetPositionAt(Index);
            const bool bOnBoard = Board->GetTileDataAt(Index).bIsOccupied;
            const bool bInRegistry = Pieces->GetPieceAtPosition(Position) != nullptr;
            if (bOnBoard != bInRegistry)
            {
                Mismatches++;
                UE_LOG(LogTemp, Error,
                    TEXT("ConnectIt_BoardVisualizer: reconcile -- the board has %s at (%d, %d) but the piece registry %s (not put right here)"),
                    bOnBoard ? TEXT("a piece") : TEXT("no piece"), Position.X, Position.Y,
                    bInRegistry ? TEXT("maps one") : TEXT("doesn't"));
            }
        }
        for (const FGridPosition& Position : Pieces->GetAllPiecePositions())
        {
            if (!Board->GetTileData(Position))
            {
                Mismatches++;
                UE_LOG(LogTemp, Error,
                    TEXT("ConnectIt_BoardVisualizer: reconcile -- the piece registry maps a piece at (%d, %d), where the board has no tile (not put right here)"),
                    Position.X, Position.Y);
            }
        }
    }
    LastEventNote.Reset();
    return Mismatches;
}

AConnectIt_PieceBase* AConnectIt_BoardVisualizer::SpawnPieceAt(const FGridPosition& Position, int32 Faction)
{
    UConnectIt_PieceRegistry* Pieces = GetPieceRegistry();
    if (!Pieces) return nullptr;

    // Already there (e.g. placed by older visuals): take it as it is
    if (AConnectIt_PieceBase* Existing = Pieces->GetPieceAt(Position))
    {
        Existing->InitialisePiece(Faction);
        return Existing;
    }

    const UConnectIt_LevelConfigDataAsset* LevelConfig = AConnectIt_WorldSettings::FindLevelConfig(this);
    const TSubclassOf<AConnectIt_PieceBase> PieceClass = LevelConfig ? LevelConfig->PieceActorClass : nullptr;
    if (!PieceClass)
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardVisualizer: no Piece Actor Class on the level config -- no piece at (%d, %d)"),
            Position.X, Position.Y);
        return nullptr;
    }

    AConnectIt_PieceBase* Piece = Cast<AConnectIt_PieceBase>(Pieces->RetrievePiece(PieceClass));
    if (!Piece) return nullptr;

    Piece->InitialisePiece(Faction);
    Piece->SetActorLocation(GetPieceLocation(Position));
    Pieces->MapPiece(Position, Piece);
    if (AConnectIt_TileBase* Tile = GetTileAt(Position))
    {
        Pieces->ActivatePieceAt(Piece, Tile);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ConnectIt_BoardVisualizer: a piece at (%d, %d) has no tile -- not activated"),
            Position.X, Position.Y);
    }
    return Piece;
}

void AConnectIt_BoardVisualizer::TakeLeavingPiece(const FGridPosition& Position)
{
    UConnectIt_PieceRegistry* Pieces = GetPieceRegistry();
    if (AGridPieceBase* Piece = Pieces ? Pieces->UnmapPiece(Position) : nullptr)
    {
        LeavingPieces.Add(Position, Piece);
    }
}

const FConnectItBoardState* AConnectIt_BoardVisualizer::GetDisplayBoard() const
{
    const UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    return BoardState ? &BoardState->GetDisplayBoard() : nullptr;
}

// --- Queries ---

UConnectIt_TileRegistry* AConnectIt_BoardVisualizer::GetTileRegistry() const
{
    return UConnectIt_GameUtilityLibrary::GetTileRegistry(this);
}

UConnectIt_PieceRegistry* AConnectIt_BoardVisualizer::GetPieceRegistry() const
{
    return UConnectIt_GameUtilityLibrary::GetPieceRegistry(this);
}

AConnectIt_TileBase* AConnectIt_BoardVisualizer::GetTileAt(FGridPosition Position) const
{
    const UConnectIt_TileRegistry* Tiles = GetTileRegistry();
    return Tiles ? Tiles->GetTileAt(Position) : nullptr;
}

AConnectIt_PieceBase* AConnectIt_BoardVisualizer::GetPieceAt(FGridPosition Position) const
{
    const UConnectIt_PieceRegistry* PieceRegistry = GetPieceRegistry();
    return PieceRegistry ? PieceRegistry->GetPieceAt(Position) : nullptr;
}

FVector AConnectIt_BoardVisualizer::GetPieceLocation(FGridPosition Position) const
{
    const UConnectIt_TileRegistry* Tiles = GetTileRegistry();
    if (const AGridTileBase* Tile = Tiles ? Tiles->GetTileAtPosition(Position) : nullptr)
    {
        return Tile->GetActorLocation() + PieceOffset;
    }
    const UGridDefinition* Grid = Tiles ? Tiles->GridDefinition.Get() : nullptr;
    return (Grid ? Grid->GridPositionToWorld(Position) : FVector::ZeroVector) + PieceOffset;
}

TArray<AConnectIt_PieceBase*> AConnectIt_BoardVisualizer::GetArrivingPieces() const
{
    TArray<AConnectIt_PieceBase*> Found;
    for (AConnectIt_PieceBase* Piece : ArrivingPieces)
    {
        if (IsValid(Piece)) Found.Add(Piece);
    }
    return Found;
}

TArray<AConnectIt_PieceBase*> AConnectIt_BoardVisualizer::GetLeavingPieces() const
{
    TArray<AConnectIt_PieceBase*> Found;
    for (const TPair<FGridPosition, TObjectPtr<AGridPieceBase>>& Leaving : LeavingPieces)
    {
        if (AConnectIt_PieceBase* Piece = Cast<AConnectIt_PieceBase>(Leaving.Value)) Found.Add(Piece);
    }
    return Found;
}

TArray<AConnectIt_TileBase*> AConnectIt_BoardVisualizer::GetLeavingTiles() const
{
    TArray<AConnectIt_TileBase*> Found;
    for (AGridTileBase* Tile : LeavingTiles)
    {
        if (AConnectIt_TileBase* ConnectItTile = Cast<AConnectIt_TileBase>(Tile)) Found.Add(ConnectItTile);
    }
    return Found;
}

int32 AConnectIt_BoardVisualizer::GetActiveBoardEventIndex() const
{
    return UConnectIt_BoardEventLibrary::GetActiveBoardEventIndex(this);
}

bool AConnectIt_BoardVisualizer::GetBoardEventCause(FInstancedStruct& OutCause) const
{
    return UConnectIt_BoardEventLibrary::GetBoardEventCause(this, OutCause);
}

// --- Defaults: the actors' hooks ---

namespace
{
    // The faction of the piece at Position on Board (-1: none)
    int32 FactionAt(const FConnectItBoardState* Board, const FGridPosition& Position)
    {
        const FConnectItTileData* Tile = Board ? Board->GetTileData(Position) : nullptr;
        return Tile ? Tile->FactionPiece : INDEX_NONE;
    }

    // The multiplier at Position on Board (1 if none)
    float MultiplierAt(const FConnectItBoardState* Board, const FGridPosition& Position)
    {
        const FConnectItTileData* Tile = Board ? Board->GetTileData(Position) : nullptr;
        return Tile ? Tile->Multiplier : 1.f;
    }

    // An event of type T made for a hook another kind of event leads to: its
    // cause is that event, and it plays where that event plays
    template <typename T>
    T MadeFor(const FConnectItBoardEvent& Source)
    {
        T Made;
        Made.CauseIndex = Source.PlayIndex;
        Made.PlayIndex = Source.PlayIndex;
        Made.PlayChangeId = Source.PlayChangeId;
        return Made;
    }

    // A Multiplier Changed for Position, made for Source
    FConnectItBoardEvent_MultiplierChanged MultiplierMadeFor(const FConnectItBoardEvent& Source,
        const FGridPosition& Position, float Previous, float New)
    {
        FConnectItBoardEvent_MultiplierChanged Made = MadeFor<FConnectItBoardEvent_MultiplierChanged>(Source);
        Made.Position = Position;
        Made.PreviousMultiplier = Previous;
        Made.NewMultiplier = New;
        return Made;
    }
}

void AConnectIt_BoardVisualizer::PlaceArrivingPieces(const FConnectItBoardEvent& Source, UConnectIt_VisualHandle* Handle)
{
    for (AConnectIt_PieceBase* Piece : GetArrivingPieces())
    {
        FConnectItBoardEvent_PiecePlaced Placed = MadeFor<FConnectItBoardEvent_PiecePlaced>(Source);
        Placed.Position = Piece->GetGridPosition();
        Placed.Faction = Piece->GetFactionID();
        Piece->OnPlaced(Placed, Handle);
    }
}

void AConnectIt_BoardVisualizer::OnBoardSeeded_Implementation(const FConnectItBoardEvent_BoardSeeded& Event, UConnectIt_VisualHandle* Handle)
{
    PlaceArrivingPieces(Event, Handle);
}

void AConnectIt_BoardVisualizer::OnPiecePlaced_Implementation(const FConnectItBoardEvent_PiecePlaced& Event, UConnectIt_VisualHandle* Handle)
{
    if (AConnectIt_TileBase* Tile = GetTileAt(Event.Position))
    {
        Tile->OnPieceEnter(FInstancedStruct::Make(Event), Event.Faction, Handle);
    }
    if (AConnectIt_PieceBase* Piece = GetPieceAt(Event.Position))
    {
        Piece->OnPlaced(Event, Handle);
    }
}

void AConnectIt_BoardVisualizer::OnPiecesSwapped_Implementation(const FConnectItBoardEvent_PiecesSwapped& Event, UConnectIt_VisualHandle* Handle)
{
    // The display board is still the board before the swap
    const int32 FactionA = FactionAt(GetDisplayBoard(), Event.PositionA);
    const int32 FactionB = FactionAt(GetDisplayBoard(), Event.PositionB);
    const FInstancedStruct Copy = FInstancedStruct::Make(Event);
    if (AConnectIt_TileBase* TileA = GetTileAt(Event.PositionA))
    {
        TileA->OnPieceExit(Copy, FactionA, Handle);
        TileA->OnPieceEnter(Copy, FactionB, Handle);
    }
    if (AConnectIt_TileBase* TileB = GetTileAt(Event.PositionB))
    {
        TileB->OnPieceExit(Copy, FactionB, Handle);
        TileB->OnPieceEnter(Copy, FactionA, Handle);
    }
    // The registry has swapped them: each piece's position is its new one
    if (AConnectIt_PieceBase* AtA = GetPieceAt(Event.PositionA))
    {
        AtA->OnSwapped(Event, Handle);
    }
    if (AConnectIt_PieceBase* AtB = GetPieceAt(Event.PositionB))
    {
        AtB->OnSwapped(Event, Handle);
    }
}

void AConnectIt_BoardVisualizer::OnPieceCaptured_Implementation(const FConnectItBoardEvent_PieceCaptured& Event, UConnectIt_VisualHandle* Handle)
{
    if (AConnectIt_TileBase* Tile = GetTileAt(Event.Position))
    {
        Tile->OnPieceEnter(FInstancedStruct::Make(Event), Event.CapturingFaction, Handle);
    }
    if (AConnectIt_PieceBase* Piece = GetPieceAt(Event.Position))
    {
        Piece->OnCaptured(Event, Handle);
    }
}

void AConnectIt_BoardVisualizer::OnPieceRemoved_Implementation(const FConnectItBoardEvent_PieceRemoved& Event, UConnectIt_VisualHandle* Handle)
{
    if (AConnectIt_TileBase* Tile = GetTileAt(Event.Position))
    {
        Tile->OnPieceExit(FInstancedStruct::Make(Event), Event.RemovedFaction, Handle);
    }
    const TObjectPtr<AGridPieceBase>* Leaving = LeavingPieces.Find(Event.Position);
    if (AConnectIt_PieceBase* Piece = Leaving ? Cast<AConnectIt_PieceBase>(*Leaving) : nullptr)
    {
        Piece->OnRemoved(Event, Handle);
    }
}

void AConnectIt_BoardVisualizer::OnPieceMoved_Implementation(const FConnectItBoardEvent_PieceMoved& Event, UConnectIt_VisualHandle* Handle)
{
    const FInstancedStruct Copy = FInstancedStruct::Make(Event);
    if (AConnectIt_TileBase* From = GetTileAt(Event.From))
    {
        From->OnPieceExit(Copy, Event.Faction, Handle);
    }
    if (AConnectIt_TileBase* To = GetTileAt(Event.To))
    {
        To->OnPieceEnter(Copy, Event.Faction, Handle);
    }
    if (AConnectIt_PieceBase* Piece = GetPieceAt(Event.To))
    {
        Piece->OnMoved(Event, Handle);
    }
}

void AConnectIt_BoardVisualizer::OnBoardShifted_Implementation(const FConnectItBoardEvent_BoardShifted& Event, UConnectIt_VisualHandle* Handle)
{
    // Not a Piece Enter / Exit: the pieces ride their tiles (a Piece Moved
    // made for each)
    for (int32 Index = 0; Index < Event.StartPositions.Num() && Index < Event.EndPositions.Num(); Index++)
    {
        if (AConnectIt_PieceBase* Piece = GetPieceAt(Event.EndPositions[Index]))
        {
            FConnectItBoardEvent_PieceMoved Moved = MadeFor<FConnectItBoardEvent_PieceMoved>(Event);
            Moved.From = Event.StartPositions[Index];
            Moved.To = Event.EndPositions[Index];
            Moved.Faction = Piece->GetFactionID();
            Piece->OnMoved(Moved, Handle);
        }
    }
}

void AConnectIt_BoardVisualizer::OnScored_Implementation(const FConnectItBoardEvent_Scored& Event, UConnectIt_VisualHandle* Handle)
{
    for (int32 Index = 0; Index < Event.Positions.Num() && Index < Event.NewMultipliers.Num(); Index++)
    {
        const FGridPosition& Position = Event.Positions[Index];
        if (AConnectIt_TileBase* Tile = GetTileAt(Position))
        {
            Tile->OnMultiplierChanged(MultiplierMadeFor(Event, Position,
                MultiplierAt(GetDisplayBoard(), Position), Event.NewMultipliers[Index]), Handle);
        }
    }
    for (const FGridPosition& Cleared : Event.ClearedPositions)
    {
        const TObjectPtr<AGridPieceBase>* Leaving = LeavingPieces.Find(Cleared);
        if (AConnectIt_PieceBase* Piece = Leaving ? Cast<AConnectIt_PieceBase>(*Leaving) : nullptr)
        {
            Piece->OnClearedByScore(Event, Handle);
        }
    }
}

void AConnectIt_BoardVisualizer::OnMultiplierChanged_Implementation(const FConnectItBoardEvent_MultiplierChanged& Event, UConnectIt_VisualHandle* Handle)
{
    if (AConnectIt_TileBase* Tile = GetTileAt(Event.Position))
    {
        Tile->OnMultiplierChanged(Event, Handle);
    }
}

void AConnectIt_BoardVisualizer::OnTileMultiplierDestroyed_Implementation(const FConnectItBoardEvent_TileMultiplierDestroyed& Event, UConnectIt_VisualHandle* Handle)
{
    if (AConnectIt_TileBase* Tile = GetTileAt(Event.Position))
    {
        Tile->OnMultiplierChanged(MultiplierMadeFor(Event, Event.Position, Event.PreviousMultiplier, Event.NewMultiplier), Handle);
    }
}

void AConnectIt_BoardVisualizer::OnTileLockChanged_Implementation(const FConnectItBoardEvent_TileLockChanged& Event, UConnectIt_VisualHandle* Handle)
{
    if (AConnectIt_TileBase* Tile = GetTileAt(Event.Position))
    {
        Tile->OnLockChanged(Event, Handle);
    }
}

void AConnectIt_BoardVisualizer::OnTileActiveToggled_Implementation(const FConnectItBoardEvent_TileActiveToggled& Event, UConnectIt_VisualHandle* Handle)
{
    if (AConnectIt_TileBase* Tile = GetTileAt(Event.Position))
    {
        Tile->OnActiveChanged(Event, Handle);
    }
}

void AConnectIt_BoardVisualizer::OnChainPartnerChanged_Implementation(const FConnectItBoardEvent_ChainPartnerChanged& Event, UConnectIt_VisualHandle* Handle)
{
    if (AConnectIt_TileBase* Chained = GetTileAt(Event.Position))
    {
        Chained->OnChainPartnerChanged(Event, Handle);
    }
    if (Event.bHasPartner)
    {
        if (AConnectIt_TileBase* Partner = GetTileAt(Event.PartnerPosition))
        {
            Partner->OnChainPartnerChanged(Event, Handle);
        }
    }
}

void AConnectIt_BoardVisualizer::OnTilePowerTriggered_Implementation(const FConnectItBoardEvent_TilePowerTriggered& Event, UConnectIt_VisualHandle* Handle)
{
    if (AConnectIt_TileBase* Tile = GetTileAt(Event.Position))
    {
        Tile->OnTilePowerTriggered(Event, Handle);
    }
}

void AConnectIt_BoardVisualizer::OnTilesAdded_Implementation(const FConnectItBoardEvent_TilesAdded& Event, UConnectIt_VisualHandle* Handle)
{
    PlaceArrivingPieces(Event, Handle);
}

void AConnectIt_BoardVisualizer::OnTilesRemoved_Implementation(const FConnectItBoardEvent_TilesRemoved& Event, UConnectIt_VisualHandle* Handle)
{
    // A Piece Removed made for each piece that goes with its tile
    for (const TPair<FGridPosition, TObjectPtr<AGridPieceBase>>& Leaving : LeavingPieces)
    {
        if (AConnectIt_PieceBase* Piece = Cast<AConnectIt_PieceBase>(Leaving.Value))
        {
            FConnectItBoardEvent_PieceRemoved Removed = MadeFor<FConnectItBoardEvent_PieceRemoved>(Event);
            Removed.Position = Leaving.Key;
            Removed.RemovedFaction = Piece->GetFactionID();
            Piece->OnRemoved(Removed, Handle);
        }
    }
}

void AConnectIt_BoardVisualizer::OnLayoutChanged_Implementation(const FConnectItBoardEvent_LayoutChanged& Event, UConnectIt_VisualHandle* Handle) {}
void AConnectIt_BoardVisualizer::OnPhaseChanged_Implementation(const FConnectItBoardEvent_PhaseChanged& Event, UConnectIt_VisualHandle* Handle) {}
void AConnectIt_BoardVisualizer::OnGoalsChanged_Implementation(const FConnectItBoardEvent_GoalsChanged& Event, UConnectIt_VisualHandle* Handle) {}
void AConnectIt_BoardVisualizer::OnWinCheckpointReached_Implementation(const FConnectItBoardEvent_WinCheckpointReached& Event, UConnectIt_VisualHandle* Handle) {}
void AConnectIt_BoardVisualizer::OnLoseConditionMet_Implementation(const FConnectItBoardEvent_LoseConditionMet& Event, UConnectIt_VisualHandle* Handle) {}
void AConnectIt_BoardVisualizer::OnGameWon_Implementation(const FConnectItBoardEvent_GameWon& Event, UConnectIt_VisualHandle* Handle) {}
void AConnectIt_BoardVisualizer::OnBoardEvent_Implementation(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) {}
