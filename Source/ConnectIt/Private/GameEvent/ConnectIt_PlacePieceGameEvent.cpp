// Fill out your copyright notice in the Description page of Project Settings.


#include "GameEvent/ConnectIt_PlacePieceGameEvent.h"
#include "Tile/GridTileBase.h"
#include "Piece/GridPieceBase.h"
#include "Piece/GridPieceRegistryComponent.h"
#include "Grid/ConnectIt_GridPiece.h"


void UConnectIt_PlacePieceGameEvent::Initialise(
    UGridPieceRegistryComponent* InPieceRegistry,
    TSubclassOf<AGridPieceBase> InPieceClass,
    AGridTileBase* InTile,
    int32 InFactionSlot,
    FGridPosition InPosition)
{
    PieceRegistry = InPieceRegistry;
    PieceClass = InPieceClass;
    Tile = InTile;
    FactionSlot = InFactionSlot;
    Position = InPosition;
}

void UConnectIt_PlacePieceGameEvent::Execute_Implementation()
{
    // if (!IsValid(PieceRegistry) || !IsValid(Tile) || !PieceClass)
    // {
    //     UE_LOG(LogTemp, Error,
    //         TEXT("ConnectIt_PlacePieceGameEvent: Execute — missing required dependency"));
    //     Complete();
    //     return;
    // }
    //
    // Piece = PieceRegistry->RetrievePiece(PieceClass);
    //
    // if (!IsValid(Piece))
    // {
    //     UE_LOG(LogTemp, Error,
    //         TEXT("ConnectIt_PlacePieceGameEvent: Execute — pool retrieval failed"));
    //     Complete();
    //     return;
    // }
    //
    // // Initialize before the visual triggers, so the spawn-in visual
    // // already reflects the correct faction when it plays.
    // if (AConnectIt_GridPiece* ConnectItPiece = Cast<AConnectIt_GridPiece>(Piece))
    // {
    //     ConnectItPiece->InitialisePiece(FactionSlot, Position);
    // }
    // else
    // {
    //     UE_LOG(LogTemp, Warning,
    //         TEXT("ConnectIt_PlacePieceGameEvent: Execute — Piece is not AConnectIt_GridPiece, "
    //              "skipping faction initialisation"));
    // }
}

void UConnectIt_PlacePieceGameEvent::HandlePieceSpawned(AGridPieceBase* SpawnedPiece)
{
    
}
