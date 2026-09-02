// Fill out your copyright notice in the Description page of Project Settings.


#include "Piece/GridPieceSpawnInterpreterBase.h"

#include "Piece/GridPieceBase.h"
#include "Tile/GridTileBase.h"


void UGridPieceSpawnInterpreterBase::SpawnPiece_Implementation(AGridPieceBase* Piece, AGridTileBase* Tile)
{
    if (!IsValid(Piece) || !IsValid(Tile))
    {
        UE_LOG(LogTemp, Error,
            TEXT("GridPieceSpawnInterpreterBase: SpawnPiece — Piece or Tile invalid"));
        return;
    }

    Piece->SetActorTransform(Tile->GetActorTransform());

    Piece->OnActivationVisualComplete.AddUniqueDynamic(
        this, &UGridPieceSpawnInterpreterBase::HandleActivationVisualComplete);
}

void UGridPieceSpawnInterpreterBase::DespawnPiece_Implementation(AGridPieceBase* Piece)
{
    if (!IsValid(Piece))
    {
        UE_LOG(LogTemp, Error,
            TEXT("GridPieceSpawnInterpreterBase: DespawnPiece — Piece invalid"));
        return;
    }

    Piece->OnDeactivationVisualComplete.AddUniqueDynamic(
        this, &UGridPieceSpawnInterpreterBase::HandleDeactivationVisualComplete);
}

void UGridPieceSpawnInterpreterBase::HandleActivationVisualComplete(AGridPieceBase* Piece)
{
    if (IsValid(Piece))
    {
        Piece->OnActivationVisualComplete.RemoveDynamic(
            this, &UGridPieceSpawnInterpreterBase::HandleActivationVisualComplete);
    }

    OnPieceSpawned.Broadcast(Piece);
}

void UGridPieceSpawnInterpreterBase::HandleDeactivationVisualComplete(AGridPieceBase* Piece)
{
    if (IsValid(Piece))
    {
        Piece->OnDeactivationVisualComplete.RemoveDynamic(
            this, &UGridPieceSpawnInterpreterBase::HandleDeactivationVisualComplete);
    }

    OnPieceDespawned.Broadcast(Piece);
}
