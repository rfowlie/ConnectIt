// Fill out your copyright notice in the Description page of Project Settings.


#include "Piece/GridPieceSpawnInterpreter.h"

#include "Piece/GridPieceBase.h"
#include "Tile/GridTileBase.h"


void UGridPieceSpawnInterpreter::SpawnPiece(AGridPieceBase* Piece, AGridTileBase* Tile)
{
    if (!IsValid(Piece) || !IsValid(Tile))
    {
        UE_LOG(LogTemp, Error,
            TEXT("GridPieceSpawnInterpreter: SpawnPiece — Piece or Tile invalid"));
        return;
    }

    Piece->SetActorTransform(Tile->GetActorTransform());

    Piece->OnActivationVisualComplete.AddUniqueDynamic(
        this, &UGridPieceSpawnInterpreter::HandleActivationVisualComplete);
}

void UGridPieceSpawnInterpreter::DespawnPiece(AGridPieceBase* Piece)
{
    if (!IsValid(Piece))
    {
        UE_LOG(LogTemp, Error,
            TEXT("GridPieceSpawnInterpreter: DespawnPiece — Piece invalid"));
        return;
    }

    Piece->OnDeactivationVisualComplete.AddUniqueDynamic(
        this, &UGridPieceSpawnInterpreter::HandleDeactivationVisualComplete);
}

void UGridPieceSpawnInterpreter::HandleActivationVisualComplete(AGridPieceBase* Piece)
{
    if (IsValid(Piece))
    {
        Piece->OnActivationVisualComplete.RemoveDynamic(
            this, &UGridPieceSpawnInterpreter::HandleActivationVisualComplete);
    }

    OnPieceSpawned.Broadcast(Piece);
}

void UGridPieceSpawnInterpreter::HandleDeactivationVisualComplete(AGridPieceBase* Piece)
{
    if (IsValid(Piece))
    {
        Piece->OnDeactivationVisualComplete.RemoveDynamic(
            this, &UGridPieceSpawnInterpreter::HandleDeactivationVisualComplete);
    }

    OnPieceDespawned.Broadcast(Piece);
}
