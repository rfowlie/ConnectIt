// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/GridHoverSubsystem.h"
#include "Tile/GridTileBase.h"
#include "Piece/GridPieceBase.h"
#include "Hover/GridHoverComponent.h"


void UGridHoverSubsystem::Deinitialize()
{
    // World / actor teardown order is not guaranteed -- the registry
    // components normally unregister everything first, but if they don't,
    // drop every remaining binding here so nothing is left bound to a stale
    // object on PIE restart. RemoveDynamic on an already-unbound delegate is
    // a harmless no-op.
    for (const TObjectPtr<AGridTileBase>& Tile : RegisteredTiles)
    {
        if (IsValid(Tile))
        {
            BindTile(Tile, false);
        }
    }
    for (const TObjectPtr<AGridPieceBase>& Piece : RegisteredPieces)
    {
        if (IsValid(Piece))
        {
            Piece->OnGridPieceBeginCursorOver.RemoveDynamic(this, &ThisClass::BroadcastGridPieceHoverChanged);
        }
    }
    RegisteredTiles.Empty();
    RegisteredPieces.Empty();

    Super::Deinitialize();
}

void UGridHoverSubsystem::RegisterTile(AGridTileBase* Tile)
{
    if (!IsValid(Tile))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GridHoverSubsystem: RegisterTile called with invalid tile"));
        return;
    }

    if (RegisteredTiles.Contains(Tile)) return;
    if (!IsValid(Tile->HoverComponent))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GridHoverSubsystem: tile '%s' has no hover component -- it can't be hovered"), *Tile->GetName());
        return;
    }

    RegisteredTiles.Add(Tile);

    // chain the tile's hover component into the global relays
    BindTile(Tile, true);
    Tile->HoverComponent->SetHoverEnabled(true);
}

void UGridHoverSubsystem::UnregisterTile(AGridTileBase* Tile)
{
    if (!IsValid(Tile)) return;
    if (!RegisteredTiles.Contains(Tile)) return;

    // Disabled while still bound, so a hover on it ends for the listeners too
    if (IsValid(Tile->HoverComponent))
    {
        Tile->HoverComponent->SetHoverEnabled(false);
    }
    BindTile(Tile, false);
    RegisteredTiles.Remove(Tile);
}

void UGridHoverSubsystem::BindTile(AGridTileBase* Tile, bool bBind)
{
    UGridHoverComponent* Hover = Tile->HoverComponent;
    if (!IsValid(Hover)) return;

    if (bBind)
    {
        Hover->OnHoverBegin.AddDynamic(this, &ThisClass::HandleTileHoverBegin);
        Hover->OnHoverEnd.AddDynamic(this, &ThisClass::HandleTileHoverEnd);
        Hover->OnClicked.AddDynamic(this, &ThisClass::HandleTileClicked);
    }
    else
    {
        Hover->OnHoverBegin.RemoveDynamic(this, &ThisClass::HandleTileHoverBegin);
        Hover->OnHoverEnd.RemoveDynamic(this, &ThisClass::HandleTileHoverEnd);
        Hover->OnClicked.RemoveDynamic(this, &ThisClass::HandleTileClicked);
    }
}

bool UGridHoverSubsystem::IsTileRegistered(const AGridTileBase* Tile) const
{
    return RegisteredTiles.Contains(Tile);
}

void UGridHoverSubsystem::RegisterPiece(AGridPieceBase* Piece)
{
    if (!IsValid(Piece))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GridHoverSubsystem: RegisterPiece called with invalid piece"));
        return;
    }

    // Re-registering an already-registered piece is expected, not a bug --
    // UGridPieceRegistryComponent registers a piece every time it's
    // retrieved from the pool, including reactivation of a previously
    // deactivated (and therefore unregistered) piece.
    if (RegisteredPieces.Contains(Piece)) return;

    RegisteredPieces.Add(Piece);

    // chain the piece's own cursor-over into the global relay
    Piece->OnGridPieceBeginCursorOver.AddDynamic(this, &ThisClass::BroadcastGridPieceHoverChanged);
}

void UGridHoverSubsystem::UnregisterPiece(AGridPieceBase* Piece)
{
    if (!IsValid(Piece)) return;
    if (!RegisteredPieces.Contains(Piece)) return;

    Piece->OnGridPieceBeginCursorOver.RemoveDynamic(this, &ThisClass::BroadcastGridPieceHoverChanged);
    RegisteredPieces.Remove(Piece);
}

bool UGridHoverSubsystem::IsPieceRegistered(const AGridPieceBase* Piece) const
{
    return RegisteredPieces.Contains(Piece);
}

void UGridHoverSubsystem::HandleTileHoverBegin(UGridHoverComponent* Component)
{
    OnGridTileHoverChanged.Broadcast(Cast<AGridTileBase>(Component->GetOwner()));
}

void UGridHoverSubsystem::HandleTileHoverEnd(UGridHoverComponent* Component)
{
    OnGridTileHoverEnded.Broadcast(Cast<AGridTileBase>(Component->GetOwner()));
}

void UGridHoverSubsystem::HandleTileClicked(UGridHoverComponent* Component)
{
    OnGridTileClicked.Broadcast(Cast<AGridTileBase>(Component->GetOwner()));
}

void UGridHoverSubsystem::BroadcastGridPieceHoverChanged(AGridPieceBase* InPiece)
{
    if (OnGridPieceHoverChanged.IsBound()) { OnGridPieceHoverChanged.Broadcast(InPiece); }
}
