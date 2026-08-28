// Fill out your copyright notice in the Description page of Project Settings.


#include "Interpreter/ConnectIt_PieceSpawnInterpreter.h"

#include "ConnectIt_GameplayTags.h"
#include "ConnectIt_Structs.h"
#include "Board/ConnectIt_BoardManager.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "GameEvent/GameEventTaskSubsystem.h"
#include "GameEvent/GameEventTask_Async.h"
#include "Piece/GridPieceBase.h"
#include "Piece/GridPieceRegistryComponent.h"
#include "Piece/GridPieceSpawnInterpreter.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridTileRegistryComponent.h"


UConnectIt_PieceSpawnInterpreter::UConnectIt_PieceSpawnInterpreter()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UConnectIt_PieceSpawnInterpreter::BeginPlay()
{
    Super::BeginPlay();

    if (!IsValid(GetOwner()))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PieceSpawnInterpreter: BeginPlay — no owner"));
        return;
    }

    PieceRegistry = GetOwner()->FindComponentByClass<UGridPieceRegistryComponent>();
    SpawnInterpreterRef = GetOwner()->FindComponentByClass<UGridPieceSpawnInterpreter>();

    if (!IsValid(PieceRegistry))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PieceSpawnInterpreter: BeginPlay — no "
                 "UGridPieceRegistryComponent found on owner"));
    }

    if (!IsValid(SpawnInterpreterRef))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PieceSpawnInterpreter: BeginPlay — no "
                 "UGridPieceSpawnInterpreter found on owner"));
    }
    else
    {
        SpawnInterpreterRef->OnPieceSpawned.AddDynamic(
            this, &UConnectIt_PieceSpawnInterpreter::HandlePieceSpawned);
        SpawnInterpreterRef->OnPieceDespawned.AddDynamic(
            this, &UConnectIt_PieceSpawnInterpreter::HandlePieceDespawned);
    }

    UGameEventTaskSubsystem* GameEventSubsystem =
        GetWorld() ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>() : nullptr;

    if (!IsValid(GameEventSubsystem))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PieceSpawnInterpreter: BeginPlay — no "
                 "UGameEventTaskSubsystem in world"));
        return;
    }

    // One persistent task per watched tag -- each bound to its own handler,
    // auto-re-added to that tag's manager on every future trigger.
    PiecePlacedTask = NewObject<UGameEventTask_Async>(this);
    PiecePlacedTask->bIsPersistentTask = true;
    PiecePlacedTask->OnExecuteDelegate.BindDynamic(
        this, &UConnectIt_PieceSpawnInterpreter::HandlePiecePlacedExecute);
    GameEventSubsystem->RegisterAsyncTask(ConnectIt_Event_PiecePlaced, PiecePlacedTask, 0);

    PieceRemovedTask = NewObject<UGameEventTask_Async>(this);
    PieceRemovedTask->bIsPersistentTask = true;
    PieceRemovedTask->OnExecuteDelegate.BindDynamic(
        this, &UConnectIt_PieceSpawnInterpreter::HandlePieceRemovedExecute);
    GameEventSubsystem->RegisterAsyncTask(ConnectIt_Event_PieceRemoved, PieceRemovedTask, 0);

    PiecesSwappedTask = NewObject<UGameEventTask_Async>(this);
    PiecesSwappedTask->bIsPersistentTask = true;
    PiecesSwappedTask->OnExecuteDelegate.BindDynamic(
        this, &UConnectIt_PieceSpawnInterpreter::HandlePiecesSwappedExecute);
    GameEventSubsystem->RegisterAsyncTask(ConnectIt_Event_PiecesSwapped, PiecesSwappedTask, 0);

    PieceCapturedTask = NewObject<UGameEventTask_Async>(this);
    PieceCapturedTask->bIsPersistentTask = true;
    PieceCapturedTask->OnExecuteDelegate.BindDynamic(
        this, &UConnectIt_PieceSpawnInterpreter::HandlePieceCapturedExecute);
    GameEventSubsystem->RegisterAsyncTask(ConnectIt_Event_PieceCaptured, PieceCapturedTask, 0);

    LineScoredTask = NewObject<UGameEventTask_Async>(this);
    LineScoredTask->bIsPersistentTask = true;
    LineScoredTask->OnExecuteDelegate.BindDynamic(
        this, &UConnectIt_PieceSpawnInterpreter::HandleLineScoredExecute);
    GameEventSubsystem->RegisterAsyncTask(ConnectIt_Event_LineScored, LineScoredTask, 0);
}

void UConnectIt_PieceSpawnInterpreter::HandlePiecePlacedExecute()
{
    ProcessTagFired(ConnectIt_Event_PiecePlaced, PiecePlacedTask);
}

void UConnectIt_PieceSpawnInterpreter::HandlePieceRemovedExecute()
{
    ProcessTagFired(ConnectIt_Event_PieceRemoved, PieceRemovedTask);
}

void UConnectIt_PieceSpawnInterpreter::HandlePiecesSwappedExecute()
{
    ProcessTagFired(ConnectIt_Event_PiecesSwapped, PiecesSwappedTask);
}

void UConnectIt_PieceSpawnInterpreter::HandlePieceCapturedExecute()
{
    ProcessTagFired(ConnectIt_Event_PieceCaptured, PieceCapturedTask);
}

void UConnectIt_PieceSpawnInterpreter::HandleLineScoredExecute()
{
    ProcessTagFired(ConnectIt_Event_LineScored, LineScoredTask);
}

void UConnectIt_PieceSpawnInterpreter::ProcessTagFired(FGameplayTag FiredTag, UGameEventTask_Async* Task)
{
    ActiveTask = Task;
    PendingPieces.Reset();

    UConnectIt_BoardStateComponent* BoardStateComponent = GetOwner()
        ? GetOwner()->FindComponentByClass<UConnectIt_BoardStateComponent>()
        : nullptr;

    if (!IsValid(BoardStateComponent) || !IsValid(PieceRegistry))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PieceSpawnInterpreter: ProcessTagFired — missing "
                 "BoardStateComponent or PieceRegistry"));
        if (IsValid(ActiveTask)) { ActiveTask->CallOnComplete(); }
        ActiveTask = nullptr;
        return;
    }

    const FConnectItBoardChangeEvent& ChangeEvent = BoardStateComponent->GetChangeEvent();

    // Position + faction slot to spawn at -- resolved into an actual
    // tile/class pair further down, once every request for this firing has
    // been collected.
    struct FPendingSpawn
    {
        FGridPosition Position;
        int32 FactionSlot = -1;
    };

    TArray<FPendingSpawn> Spawns;
    TArray<FGridPosition> Despawns;

    if (FiredTag == ConnectIt_Event_PiecePlaced)
    {
        // Also fired by ForcePlacePiece (reuses this same tag -- same
        // visual outcome, a piece appears on a tile) and produces an
        // identically-shaped ChangeEvent, so no separate branch needed.
        Spawns.Add({ ChangeEvent.PlacedPosition, ChangeEvent.PlacingFactionSlot });
    }
    else if (FiredTag == ConnectIt_Event_PieceRemoved)
    {
        Despawns.Add(ChangeEvent.RemovedPosition);
    }
    else if (FiredTag == ConnectIt_Event_PieceCaptured)
    {
        Despawns.Add(ChangeEvent.CapturedPosition);
        Spawns.Add({ ChangeEvent.CapturedPosition, ChangeEvent.CapturingFactionSlot });
    }
    else if (FiredTag == ConnectIt_Event_PiecesSwapped)
    {
        // ChangeEvent carries no faction data for a swap -- read the
        // already-post-swap state instead (this fires after SetBoardState/
        // OnRep have already committed it, on both server and client).
        const FConnectItBoardState& Current = BoardStateComponent->GetCurrentState();

        Despawns.Add(ChangeEvent.SwapPositionA);
        Despawns.Add(ChangeEvent.SwapPositionB);

        if (const FConnectItTileData* DataA = Current.GetTileData(ChangeEvent.SwapPositionA))
        {
            if (DataA->IsOccupied())
            {
                Spawns.Add({ ChangeEvent.SwapPositionA, DataA->FactionPiece });
            }
        }
        if (const FConnectItTileData* DataB = Current.GetTileData(ChangeEvent.SwapPositionB))
        {
            if (DataB->IsOccupied())
            {
                Spawns.Add({ ChangeEvent.SwapPositionB, DataB->FactionPiece });
            }
        }
    }
    else if (FiredTag == ConnectIt_Event_LineScored)
    {
        // The completing tile is deliberately kept occupied by
        // IConnectIt_ScoringRule::ApplyScoring -- despawning it too would
        // desync the visual board from the real one.
        const FGridPosition CompletingPosition = ChangeEvent.bPieceCaptured
            ? ChangeEvent.CapturedPosition
            : ChangeEvent.PlacedPosition;

        for (const FGridPosition& Position : ChangeEvent.ScoringLinePositions)
        {
            if (!(Position == CompletingPosition))
            {
                Despawns.Add(Position);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PieceSpawnInterpreter: ProcessTagFired — unhandled tag '%s'"),
            *FiredTag.ToString());
    }

    // Track exactly which pieces this firing produced -- see the class
    // header comment on the known synchronous-completion limitation this
    // does NOT attempt to work around.
    for (const FPendingSpawn& Spawn : Spawns)
    {
        AGridTileBase* Tile = ResolveTile(Spawn.Position);
        const TSubclassOf<AGridPieceBase> PieceClass = GetPieceClassForFaction(Spawn.FactionSlot);

        if (!IsValid(Tile) || !PieceClass)
        {
            continue; // logged inside the two helpers above
        }

        if (AGridPieceBase* Piece = PieceRegistry->SpawnPieceAt(PieceClass, Tile))
        {
            PendingPieces.Add(Piece);
        }
    }

    for (const FGridPosition& Position : Despawns)
    {
        if (AGridPieceBase* Piece = PieceRegistry->GetPiece(Position))
        {
            PendingPieces.Add(Piece);
        }
        PieceRegistry->DespawnPieceAt(Position);
    }

    if (PendingPieces.IsEmpty())
    {
        if (IsValid(ActiveTask)) { ActiveTask->CallOnComplete(); }
        ActiveTask = nullptr;
    }
}

AGridTileBase* UConnectIt_PieceSpawnInterpreter::ResolveTile(FGridPosition Position) const
{
    const AConnectIt_BoardManager* BoardManager = Cast<AConnectIt_BoardManager>(GetOwner());

    if (!IsValid(BoardManager) || !IsValid(BoardManager->GetTileRegistry()))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PieceSpawnInterpreter: ResolveTile — owner is not "
                 "AConnectIt_BoardManager or its tile registry is missing"));
        return nullptr;
    }

    AGridTileBase* Tile = BoardManager->GetTileRegistry()->GetTileAtPosition(Position);

    if (!IsValid(Tile))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PieceSpawnInterpreter: ResolveTile — no tile at (%d,%d)"),
            Position.X, Position.Y);
    }

    return Tile;
}

TSubclassOf<AGridPieceBase> UConnectIt_PieceSpawnInterpreter::GetPieceClassForFaction(int32 FactionSlot) const
{
    if (!FactionPieceClasses.IsValidIndex(FactionSlot))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PieceSpawnInterpreter: GetPieceClassForFaction — "
                 "no class configured for faction slot %d"),
            FactionSlot);
        return nullptr;
    }

    return FactionPieceClasses[FactionSlot];
}

void UConnectIt_PieceSpawnInterpreter::HandlePieceSpawned(AGridPieceBase* Piece)
{
    HandlePieceVisualComplete(Piece);
}

void UConnectIt_PieceSpawnInterpreter::HandlePieceDespawned(AGridPieceBase* Piece)
{
    HandlePieceVisualComplete(Piece);
}

void UConnectIt_PieceSpawnInterpreter::HandlePieceVisualComplete(AGridPieceBase* Piece)
{
    // Not something this firing is waiting on -- ignore. Either a stray
    // signal, or (see the known limitation) a piece whose completion
    // already fired before it could be added to PendingPieces.
    if (PendingPieces.Remove(Piece) == 0) return;

    if (!PendingPieces.IsEmpty()) return;

    if (IsValid(ActiveTask)) { ActiveTask->CallOnComplete(); }
    ActiveTask = nullptr;
}
