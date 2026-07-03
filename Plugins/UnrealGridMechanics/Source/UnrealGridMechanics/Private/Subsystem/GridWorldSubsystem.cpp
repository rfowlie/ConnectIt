// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/GridWorldSubsystem.h"
#include "Tile/GridTileBase.h"
#include "Piece/GridPieceBase.h"


void UGridWorldSubsystem::RegisterTile(AGridTileBase* Tile, FGridPosition Position)
{
    if (!IsValid(Tile))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GridWorldSubsystem: RegisterTile called with invalid tile"));
        return;
    }

    // Warn if position is already occupied by a different tile
    if (AGridTileBase* Existing = TilesByPosition.FindRef(Position))
    {
        if (Existing != Tile)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("GridWorldSubsystem: Position (%d, %d) already occupied by %s — overwriting"),
                Position.X, Position.Y, *Existing->GetName());
        }
    }

    // Remove old position entry if tile is re-registering
    if (FGridPosition* OldPosition = PositionsByTile.Find(Tile))
    {
        TilesByPosition.Remove(*OldPosition);
    }

    TilesByPosition.Add(Position, Tile);
    PositionsByTile.Add(Tile, Position);

    OnTileRegistered.Broadcast(Tile, Position);

    UE_LOG(LogTemp, Log,
        TEXT("GridWorldSubsystem: Registered tile %s at (%d, %d)"),
        *Tile->GetName(), Position.X, Position.Y);
}

void UGridWorldSubsystem::UnregisterTile(AGridTileBase* Tile)
{
    if (!IsValid(Tile)) return;

    FGridPosition* Position = PositionsByTile.Find(Tile);
    if (!Position) return;

    OnTileUnregistered.Broadcast(Tile, *Position);

    TilesByPosition.Remove(*Position);
    PositionsByTile.Remove(Tile);
}

AGridTileBase* UGridWorldSubsystem::GetTileAtPosition(FGridPosition Position) const
{
    AGridTileBase* const* Found = TilesByPosition.Find(Position);
    return Found ? *Found : nullptr;
}

bool UGridWorldSubsystem::IsTileRegistered(AGridTileBase* Tile) const
{
    return PositionsByTile.Contains(Tile);
}

TArray<AGridTileBase*> UGridWorldSubsystem::GetAllTiles() const
{
    TArray<AGridTileBase*> Out;
    TilesByPosition.GenerateValueArray(Out);
    return Out;
}

TArray<FGridPosition> UGridWorldSubsystem::GetAllTilePositions() const
{
    TArray<FGridPosition> Out;
    TilesByPosition.GenerateKeyArray(Out);
    return Out;
}

void UGridWorldSubsystem::RegisterPiece(AGridPieceBase* Piece, FGridPosition Position)
{
    if (!IsValid(Piece))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GridWorldSubsystem: RegisterPiece called with invalid piece"));
        return;
    }

    if (AGridPieceBase* Existing = PiecesByPosition.FindRef(Position))
    {
        if (Existing != Piece)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("GridWorldSubsystem: Position (%d, %d) already has piece %s — overwriting"),
                Position.X, Position.Y, *Existing->GetName());
        }
    }

    if (FGridPosition* OldPosition = PositionsByPiece.Find(Piece))
    {
        PiecesByPosition.Remove(*OldPosition);
    }

    PiecesByPosition.Add(Position, Piece);
    PositionsByPiece.Add(Piece, Position);

    OnPieceRegistered.Broadcast(Piece, Position);
}

void UGridWorldSubsystem::UnregisterPiece(AGridPieceBase* Piece)
{
    if (!IsValid(Piece)) return;

    FGridPosition* Position = PositionsByPiece.Find(Piece);
    if (!Position) return;

    OnPieceUnregistered.Broadcast(Piece, *Position);

    PiecesByPosition.Remove(*Position);
    PositionsByPiece.Remove(Piece);
}

void UGridWorldSubsystem::UpdatePiecePosition(AGridPieceBase* Piece, FGridPosition NewPosition)
{
    if (!IsValid(Piece)) return;

    // Remove old position mapping
    if (FGridPosition* OldPosition = PositionsByPiece.Find(Piece))
    {
        PiecesByPosition.Remove(*OldPosition);
    }

    PiecesByPosition.Add(NewPosition, Piece);
    PositionsByPiece.Add(Piece, NewPosition);
}

AGridPieceBase* UGridWorldSubsystem::GetPieceAtPosition(FGridPosition Position) const
{
    AGridPieceBase* const* Found = PiecesByPosition.Find(Position);
    return Found ? *Found : nullptr;
}

bool UGridWorldSubsystem::IsPieceRegistered(AGridPieceBase* Piece) const
{
    return PositionsByPiece.Contains(Piece);
}

TArray<AGridPieceBase*> UGridWorldSubsystem::GetAllPieces() const
{
    TArray<AGridPieceBase*> Out;
    PiecesByPosition.GenerateValueArray(Out);
    return Out;
}
