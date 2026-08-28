// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/GridWorldSubsystem.h"
#include "Tile/GridTileBase.h"
#include "Piece/GridPieceBase.h"


void UGridWorldSubsystem::RegisterTile(AGridTileBase* Tile)
{
    if (!IsValid(Tile))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GridWorldSubsystem: RegisterTile called with invalid tile"));
        return;
    }

    // Warn if position is already occupied by a different tile
    if (RegisteredTiles.Contains(Tile))
    {
        UE_LOG(LogTemp, Warning,
               TEXT("GridWorldSubsystem: RegisterTile called with tile already registered"));
    }

    RegisteredTiles.Add(Tile);

    // chain delegates for global broadcast
    Tile->OnGridTileBeginCursorOver.AddDynamic(this, &ThisClass::BroadcastGridTileHoverChanged);

    OnTileRegistered.Broadcast(Tile);

    UE_LOG(LogTemp, Log,
        TEXT("GridWorldSubsystem: Registered tile"));
}

void UGridWorldSubsystem::UnregisterTile(AGridTileBase* Tile)
{
    if (!IsValid(Tile)) return;
    if (!RegisteredTiles.Contains(Tile)) return;
    OnTileUnregistered.Broadcast(Tile);
    RegisteredTiles.Remove(Tile);
}

bool UGridWorldSubsystem::IsTileRegistered(const AGridTileBase* Tile) const
{
    return RegisteredTiles.Contains(Tile);
}

TArray<AGridTileBase*> UGridWorldSubsystem::GetAllTiles() const
{
    TArray<AGridTileBase*> Out;
    RegisteredTiles.GenerateKeyArray(Out);
    return Out;
}

void UGridWorldSubsystem::RegisterPiece(AGridPieceBase* Piece)
{
    if (!IsValid(Piece))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GridWorldSubsystem: RegisterPiece called with invalid piece"));
        return;
    }

    // Re-registering an already-registered piece is expected, not a bug --
    // UGridPieceRegistryComponent registers a piece every time it's
    // retrieved from the pool, including reactivation of a previously
    // deactivated (and therefore unregistered) piece.
    if (RegisteredPieces.Contains(Piece)) return;

    RegisteredPieces.Add(Piece);
    OnPieceRegistered.Broadcast(Piece);
}

void UGridWorldSubsystem::UnregisterPiece(AGridPieceBase* Piece)
{
    if (!IsValid(Piece)) return;
    if (!RegisteredPieces.Contains(Piece)) return;
    OnPieceUnregistered.Broadcast(Piece);
    RegisteredPieces.Remove(Piece);
}

bool UGridWorldSubsystem::IsPieceRegistered(const AGridPieceBase* Piece) const
{
    return RegisteredPieces.Contains(Piece);
}

TArray<AGridPieceBase*> UGridWorldSubsystem::GetAllPieces() const
{
    // RegisteredPieces only ever contains what UGridPieceRegistryComponent
    // currently has checked out of the pool (registered on retrieval,
    // unregistered before release) -- correct by construction, no filtering
    // needed here.
    return RegisteredPieces;
}

void UGridWorldSubsystem::BroadcastGridTileHoverChanged(AGridTileBase* InTile)
{
    if (OnGridTileHoverChanged.IsBound()) { OnGridTileHoverChanged.Broadcast(InTile); }
}
