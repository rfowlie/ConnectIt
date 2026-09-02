// Fill out your copyright notice in the Description page of Project Settings.


#include "Interpreter/ConnectIt_PieceSpawnInterpreter.h"

#include "ConnectIt_GameplayTags.h"
#include "ConnectIt_Structs.h"
#include "Board/ConnectIt_BoardManager.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "GameEvent/ConnectIt_LineScoreGameEvent.h"
#include "GameEvent/ConnectIt_PlacePieceGameEvent.h"
#include "GameEvent/GameEventTaskSubsystem.h"
#include "GameEvent/GameEventTask_Async.h"
#include "GameEvent/TurnBasedGameEvent.h"
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
    const UConnectIt_BoardStateComponent* BoardStateComponent = GetOwner()
        ? GetOwner()->FindComponentByClass<UConnectIt_BoardStateComponent>()
        : nullptr;

    AGridTileBase* Tile = BoardStateComponent
        ? ResolveTile(BoardStateComponent->GetChangeEvent().PlacedPosition)
        : nullptr;

    const TSubclassOf<AGridPieceBase> PieceClass = BoardStateComponent
        ? GetPieceClassForFaction(BoardStateComponent->GetChangeEvent().PlacingFactionSlot)
        : nullptr;

    if (!BoardStateComponent || !IsValid(PieceRegistry) || !IsValid(SpawnInterpreterRef)
        || !IsValid(Tile) || !PieceClass)
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PieceSpawnInterpreter: HandlePiecePlacedExecute — "
                 "missing BoardStateComponent, PieceRegistry, SpawnInterpreterRef, Tile, or PieceClass"));
        if (IsValid(PiecePlacedTask)) { PiecePlacedTask->CallOnComplete(); }
        return;
    }

    const FConnectItBoardChangeEvent& ChangeEvent = BoardStateComponent->GetChangeEvent();

    ActivePlacePieceEvent = NewObject<UConnectIt_PlacePieceGameEvent>(this);
    ActivePlacePieceEvent->Initialise(
        PieceRegistry, SpawnInterpreterRef, PieceClass, Tile,
        ChangeEvent.PlacingFactionSlot, ChangeEvent.PlacedPosition);
    ActivePlacePieceEvent->OnComplete.AddDynamic(
        this, &UConnectIt_PieceSpawnInterpreter::HandlePlacePieceEventComplete);
    ActivePlacePieceEvent->Execute();
}

void UConnectIt_PieceSpawnInterpreter::HandlePlacePieceEventComplete(UTurnBasedGameEvent* Event)
{
    ActivePlacePieceEvent = nullptr;
    if (IsValid(PiecePlacedTask)) { PiecePlacedTask->CallOnComplete(); }
}

void UConnectIt_PieceSpawnInterpreter::HandlePieceRemovedExecute()
{
    ActiveTask = PieceRemovedTask;
    PendingPieces.Reset();

    const UConnectIt_BoardStateComponent* BoardStateComponent = GetOwner()
        ? GetOwner()->FindComponentByClass<UConnectIt_BoardStateComponent>()
        : nullptr;

    if (BoardStateComponent)
    {
        const FConnectItBoardChangeEvent& ChangeEvent = BoardStateComponent->GetChangeEvent();
        TArray<FGridPosition> Despawns;
        Despawns.Add(ChangeEvent.RemovedPosition);
        HandleDespawn(Despawns);
    }

    CompleteTaskIfNothingPending();
}

void UConnectIt_PieceSpawnInterpreter::HandlePiecesSwappedExecute()
{
    ActiveTask = PiecesSwappedTask;
    PendingPieces.Reset();

    const UConnectIt_BoardStateComponent* BoardStateComponent = GetOwner()
        ? GetOwner()->FindComponentByClass<UConnectIt_BoardStateComponent>()
        : nullptr;

    if (BoardStateComponent)
    {
        const FConnectItBoardChangeEvent& ChangeEvent = BoardStateComponent->GetChangeEvent();

        // ChangeEvent carries no faction data for a swap -- read the
        // already-post-swap state instead (this fires after SetBoardState/
        // OnRep have already committed it, on both server and client).
        const FConnectItBoardState& Current = BoardStateComponent->GetCurrentState();

        TArray<FPendingSpawn> Spawns;
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
        HandleSpawn(Spawns);

        TArray<FGridPosition> Despawns;
        Despawns.Add(ChangeEvent.SwapPositionA);
        Despawns.Add(ChangeEvent.SwapPositionB);
        HandleDespawn(Despawns);
    }

    CompleteTaskIfNothingPending();
}

void UConnectIt_PieceSpawnInterpreter::HandlePieceCapturedExecute()
{
    ActiveTask = PieceCapturedTask;
    PendingPieces.Reset();

    const UConnectIt_BoardStateComponent* BoardStateComponent = GetOwner()
        ? GetOwner()->FindComponentByClass<UConnectIt_BoardStateComponent>()
        : nullptr;

    if (BoardStateComponent)
    {
        const FConnectItBoardChangeEvent& ChangeEvent = BoardStateComponent->GetChangeEvent();

        TArray<FPendingSpawn> Spawns;
        Spawns.Add({ ChangeEvent.CapturedPosition, ChangeEvent.CapturingFactionSlot });
        HandleSpawn(Spawns);

        TArray<FGridPosition> Despawns;
        Despawns.Add(ChangeEvent.CapturedPosition);
        HandleDespawn(Despawns);
    }

    CompleteTaskIfNothingPending();
}

void UConnectIt_PieceSpawnInterpreter::HandleLineScoredExecute()
{
    const UConnectIt_BoardStateComponent* BoardStateComponent = GetOwner()
        ? GetOwner()->FindComponentByClass<UConnectIt_BoardStateComponent>()
        : nullptr;

    if (!BoardStateComponent || !IsValid(PieceRegistry) || !IsValid(SpawnInterpreterRef))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PieceSpawnInterpreter: HandleLineScoredExecute — "
                 "missing BoardStateComponent, PieceRegistry, or SpawnInterpreterRef"));
        if (IsValid(LineScoredTask)) { LineScoredTask->CallOnComplete(); }
        return;
    }

    const FConnectItBoardChangeEvent& ChangeEvent = BoardStateComponent->GetChangeEvent();

    // The completing tile is deliberately kept occupied by
    // IConnectIt_ScoringRule::ApplyScoring -- despawning it too would
    // desync the visual board from the real one.
    const FGridPosition CompletingPosition = ChangeEvent.bPieceCaptured
        ? ChangeEvent.CapturedPosition
        : ChangeEvent.PlacedPosition;

    TArray<FGridPosition> Despawns;
    for (const FGridPosition& Position : ChangeEvent.ScoringLinePositions)
    {
        if (!(Position == CompletingPosition))
        {
            Despawns.Add(Position);
        }
    }

    ActiveLineScoreEvent = NewObject<UConnectIt_LineScoreGameEvent>(this);
    ActiveLineScoreEvent->Initialise(PieceRegistry, SpawnInterpreterRef, Despawns);
    ActiveLineScoreEvent->OnComplete.AddDynamic(
        this, &UConnectIt_PieceSpawnInterpreter::HandleLineScoreEventComplete);
    ActiveLineScoreEvent->Execute();
}

void UConnectIt_PieceSpawnInterpreter::HandleLineScoreEventComplete(UTurnBasedGameEvent* Event)
{
    ActiveLineScoreEvent = nullptr;
    if (IsValid(LineScoredTask)) { LineScoredTask->CallOnComplete(); }
}

void UConnectIt_PieceSpawnInterpreter::HandleSpawn(TArray<FPendingSpawn> Spawns)
{
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
}

void UConnectIt_PieceSpawnInterpreter::HandleDespawn(TArray<FGridPosition> Despawns)
{

    for (const FGridPosition& Position : Despawns)
    {
        if (AGridPieceBase* Piece = PieceRegistry->GetPiece(Position))
        {
            PendingPieces.Add(Piece);
        }
        PieceRegistry->DespawnPieceAt(Position);
    }
}

void UConnectIt_PieceSpawnInterpreter::CompleteTaskIfNothingPending()
{
    if (!PendingPieces.IsEmpty()) return;

    if (IsValid(ActiveTask)) { ActiveTask->CallOnComplete(); }
    ActiveTask = nullptr;
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
    // signal, or (for the still-procedural tags only) a piece whose
    // completion already fired before it could be added to PendingPieces.
    if (PendingPieces.Remove(Piece) == 0) return;

    CompleteTaskIfNothingPending();
}
