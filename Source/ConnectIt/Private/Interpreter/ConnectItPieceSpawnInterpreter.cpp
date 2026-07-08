// Fill out your copyright notice in the Description page of Project Settings.


#include "Interpreter/ConnectItPieceSpawnInterpreter.h"

#include "Board/ConnectItBoardStateComponent.h"
#include "Grid/ConnectIt_GridPiece.h"
#include "Subsystem/GridWorldSubsystem.h"


void UConnectItPieceSpawnInterpreter::BeginPlay()
{
    Super::BeginPlay();

    // Pool created on ALL machines -- server and clients
    // Server pool is unused (server never spawns visual pieces)
    // but creating it everywhere avoids HasAuthority checks
    if (IsValid(PieceActorClass))
    {
        PiecePool = UActorPool::Create(
            GetWorld(),
            PieceActorClass,
            PoolInitialSize
        );

        UE_LOG(LogTemp, Log,
            TEXT("ConnectItPieceSpawnInterpreter: Pool created — "
                 "%d pieces pre-warmed"),
            PoolInitialSize);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectItPieceSpawnInterpreter: No PieceActorClass set "
                 "— pieces will not spawn"));
    }
}

void UConnectItPieceSpawnInterpreter::OnBoardStateChanged_Implementation(
    const UBoardStateComponentBase* Component)
{
    // Server does not spawn visual pieces
    if (GetOwner()->HasAuthority()) return;

    const UConnectItBoardStateComponent* ConnectItComp =
        Cast<UConnectItBoardStateComponent>(Component);
    if (!IsValid(ConnectItComp)) return;
    
    const FConnectItBoardStateSnapshot Snapshot = *ConnectItComp->GetBoardSnapshot();
    
    if (!IsValid(PiecePool)) return;

    for (int32 i = 0; i < Snapshot.CurrentState.NumTiles(); i++)
    {
        const FGridPosition Position    = Snapshot.CurrentState.GetPositionAt(i);
        const FConnectItTileData& CurrentTileData = Snapshot.CurrentState.GetTileDataAt(i);
        
        const FConnectItTileData* PreviousTileData =
            Snapshot.PreviousState.GetTileData(Position);

        const bool bWasOccupied = PreviousTileData
            && PreviousTileData->IsOccupied();
        const bool bIsOccupied  = CurrentTileData.IsOccupied();

        if (!bWasOccupied && bIsOccupied)
        {
            // Tile became occupied -- spawn piece
            SpawnPieceAt(Position, CurrentTileData.FactionPiece);
        }
        else if (bWasOccupied && !bIsOccupied)
        {
            // Tile became empty -- return piece to pool
            ReturnPieceToPool(Position);
        }
        else if (bWasOccupied && bIsOccupied
            && PreviousTileData->FactionPiece != CurrentTileData.FactionPiece)
        {
            // Faction changed on occupied tile (edge case -- shift moved a piece)
            ReturnPieceToPool(Position);
            SpawnPieceAt(Position, CurrentTileData.FactionPiece);
        }
    }

    // Return pieces for positions that no longer exist in current state
    // (handles tiles removed by mutations)
    TArray<FGridPosition> ToRemove;
    for (const auto& [Position, Piece] : ActivePieces)
    {
        if (!Snapshot.CurrentState.TilePositions.Contains(Position))
        {
            ToRemove.Add(Position);
        }
    }
    for (const FGridPosition& Position : ToRemove)
    {
        ReturnPieceToPool(Position);
    }
}

void UConnectItPieceSpawnInterpreter::SpawnPieceAt(
    const FGridPosition& Position,
    int32 FactionID)
{
    if (!IsValid(PiecePool)) return;

    TArray<AActor*> Pooled = PiecePool->GetObjects(1);
    if (Pooled.IsEmpty()) return;

    AConnectIt_GridPiece* Piece = Cast<AConnectIt_GridPiece>(Pooled[0]);
    if (!IsValid(Piece))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectItPieceSpawnInterpreter: Pooled actor is not "
                 "AConnectItGridPiece"));
        return;
    }

    Piece->SetActorLocation(GetWorldPositionForTile(Position));
    Piece->InitialisePiece(FactionID, Position);
    ActivePieces.Add(Position, Piece);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectItPieceSpawnInterpreter: Spawned piece at (%d,%d) "
             "faction %d"),
        Position.X, Position.Y, FactionID);
}

void UConnectItPieceSpawnInterpreter::ReturnPieceToPool(
    const FGridPosition& Position)
{
    const TObjectPtr<AConnectIt_GridPiece>* Found = ActivePieces.Find(Position);
    if (!Found) return;

    if (IsValid(PiecePool) && IsValid(*Found))
    {
        PiecePool->ReleaseObject(*Found);
    }

    ActivePieces.Remove(Position);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectItPieceSpawnInterpreter: Returned piece at (%d,%d) "
             "to pool"),
        Position.X, Position.Y);
}

FVector UConnectItPieceSpawnInterpreter::GetWorldPositionForTile(
    const FGridPosition& Position) const
{
    UWorld* World = GetWorld();
    if (!IsValid(World)) return FVector::ZeroVector;

    UGridWorldSubsystem* GridSub =
        World->GetSubsystem<UGridWorldSubsystem>();
    if (!IsValid(GridSub)) return FVector::ZeroVector;

    // AGridTileBase* Tile = GridSub->GetTileAtPosition(Position);
    // return IsValid(Tile)
    //     ? Tile->GetActorLocation()
    //     : FVector::ZeroVector;

    return FVector();
}

