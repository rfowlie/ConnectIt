// Fill out your copyright notice in the Description page of Project Settings.

// GridHoverSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GridMechanicsBaseStructs.h"
#include "GridHoverSubsystem.generated.h"


class AGridTileBase;
class AGridPieceBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridHoverSubsystemTileDelegate, AGridTileBase*, Tile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridHoverSubsystemPieceDelegate, AGridPieceBase*, Piece);

/*
 * Relays mouse-hover for the tiles and pieces that have been registered with
 * it -- and only those. Registration is driven by the tile / piece registry
 * components (UGridTileRegistryComponent, UGridPieceRegistryComponent), never
 * by the actors themselves, so pooled pieces that are currently inactive are
 * not registered and cannot fire hover.
 *
 * This subsystem does no hit-testing of its own: each registered actor
 * broadcasts its own OnGrid{Tile,Piece}BeginCursorOver (wired from the
 * actor's native "Begin Cursor Over" event), and this subsystem chains those
 * into a single OnGrid{Tile,Piece}HoverChanged consumers can bind once.
 */
UCLASS()
class UNREALGRIDMECHANICS_API UGridHoverSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:

    virtual void Deinitialize() override;

    // --- Tile Registration (called by UGridTileRegistryComponent) ---

    UFUNCTION(BlueprintCallable, Category = "Grid|Hover")
    void RegisterTile(AGridTileBase* Tile);

    UFUNCTION(BlueprintCallable, Category = "Grid|Hover")
    void UnregisterTile(AGridTileBase* Tile);

    UFUNCTION(BlueprintPure, Category = "Grid|Hover")
    bool IsTileRegistered(const AGridTileBase* Tile) const;

    // --- Piece Registration (called by UGridPieceRegistryComponent) ---

    UFUNCTION(BlueprintCallable, Category = "Grid|Hover")
    void RegisterPiece(AGridPieceBase* Piece);

    UFUNCTION(BlueprintCallable, Category = "Grid|Hover")
    void UnregisterPiece(AGridPieceBase* Piece);

    UFUNCTION(BlueprintPure, Category = "Grid|Hover")
    bool IsPieceRegistered(const AGridPieceBase* Piece) const;

    // --- Hover Relay ---

    UPROPERTY(BlueprintAssignable, Category = "Grid|Hover")
    FGridHoverSubsystemTileDelegate OnGridTileHoverChanged;

    UPROPERTY(BlueprintAssignable, Category = "Grid|Hover")
    FGridHoverSubsystemPieceDelegate OnGridPieceHoverChanged;

private:

    UPROPERTY()
    TSet<TObjectPtr<AGridTileBase>> RegisteredTiles;

    UPROPERTY()
    TSet<TObjectPtr<AGridPieceBase>> RegisteredPieces;

    UFUNCTION()
    void BroadcastGridTileHoverChanged(AGridTileBase* InTile);

    UFUNCTION()
    void BroadcastGridPieceHoverChanged(AGridPieceBase* InPiece);

};
