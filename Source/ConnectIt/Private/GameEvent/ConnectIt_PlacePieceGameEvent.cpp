// Fill out your copyright notice in the Description page of Project Settings.


#include "GameEvent/ConnectIt_PlacePieceGameEvent.h"

#include "Grid/ConnectIt_GridPiece.h"
#include "Piece/GridPieceBase.h"
#include "Piece/GridPieceRegistryComponent.h"
#include "Piece/GridPieceSpawnInterpreter.h"
#include "Tile/GridTileBase.h"


void UConnectIt_PlacePieceGameEvent::Initialise(
    UGridPieceRegistryComponent* InPieceRegistry,
    UGridPieceSpawnInterpreter* InSpawnInterpreter,
    TSubclassOf<AGridPieceBase> InPieceClass,
    AGridTileBase* InTile,
    int32 InFactionSlot,
    FGridPosition InPosition)
{
    PieceRegistry = InPieceRegistry;
    SpawnInterpreter = InSpawnInterpreter;
    PieceClass = InPieceClass;
    Tile = InTile;
    FactionSlot = InFactionSlot;
    Position = InPosition;
}

void UConnectIt_PlacePieceGameEvent::Execute_Implementation()
{
    if (!IsValid(PieceRegistry) || !IsValid(SpawnInterpreter) || !IsValid(Tile) || !PieceClass)
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PlacePieceGameEvent: Execute — missing required dependency"));
        Complete();
        return;
    }

    Piece = PieceRegistry->RetrievePiece(PieceClass);

    if (!IsValid(Piece))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_PlacePieceGameEvent: Execute — pool retrieval failed"));
        Complete();
        return;
    }

    // Initialize before the visual triggers, so the spawn-in visual
    // already reflects the correct faction when it plays.
    if (AConnectIt_GridPiece* ConnectItPiece = Cast<AConnectIt_GridPiece>(Piece))
    {
        ConnectItPiece->InitialisePiece(FactionSlot, Position);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_PlacePieceGameEvent: Execute — Piece is not AConnectIt_GridPiece, "
                 "skipping faction initialisation"));
    }

    SpawnInterpreter->OnPieceSpawned.AddDynamic(
        this, &UConnectIt_PlacePieceGameEvent::HandlePieceSpawned);
    PieceRegistry->ActivatePieceAt(Piece, Tile);
}

void UConnectIt_PlacePieceGameEvent::HandlePieceSpawned(AGridPieceBase* SpawnedPiece)
{
    // OnPieceSpawned is shared across every concurrently-in-flight
    // consumer of it -- ignore anything that isn't the piece this event
    // retrieved.
    if (SpawnedPiece != Piece) return;

    if (IsValid(SpawnInterpreter))
    {
        SpawnInterpreter->OnPieceSpawned.RemoveDynamic(
            this, &UConnectIt_PlacePieceGameEvent::HandlePieceSpawned);
    }

    Complete();
}
