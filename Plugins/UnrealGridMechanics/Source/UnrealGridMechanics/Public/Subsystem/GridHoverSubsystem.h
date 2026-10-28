// Fill out your copyright notice in the Description page of Project Settings.

// GridHoverSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GridMechanicsBaseStructs.h"
#include "GridHoverSubsystem.generated.h"


class AGridTileBase;
class AGridPieceBase;
class UGridHoverComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridHoverSubsystemTileDelegate, AGridTileBase*, Tile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridHoverSubsystemPieceDelegate, AGridPieceBase*, Piece);

/*
 * Relays mouse hover and clicks for the tiles that have been registered with
 * it -- and only those. Registration is driven by the tile registry
 * component (UGridTileRegistryComponent), never by the actors themselves, so
 * only board tiles report.
 *
 * This subsystem does no hit-testing and decides nothing: each registered
 * tile's UGridHoverComponent raises hover begin / end and click from the
 * tile's cursor events (in C++), and this subsystem chains them into
 * OnGridTileHoverChanged / OnGridTileHoverEnded / OnGridTileClicked, which
 * listeners (the player actions) bind once.
 *
 * Pieces are not hovered: the piece on a tile is found through the hovered
 * tile, and pieces ignore the cursor trace (AGridPieceBase). The piece relay
 * is deprecated, kept working for one release.
 */
UCLASS()
class UNREALGRIDMECHANICS_API UGridHoverSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:

    virtual void Deinitialize() override;

    // --- Tile Registration (called by UGridTileRegistryComponent) ---

    // Binds the tile's hover component's events and enables it
    UFUNCTION(BlueprintCallable, Category = "Grid|Hover")
    void RegisterTile(AGridTileBase* Tile);

    // Disables the tile's hover component (ending a hover on it, which is
    // still relayed) and unbinds it
    UFUNCTION(BlueprintCallable, Category = "Grid|Hover")
    void UnregisterTile(AGridTileBase* Tile);

    UFUNCTION(BlueprintPure, Category = "Grid|Hover")
    bool IsTileRegistered(const AGridTileBase* Tile) const;

    // --- Piece Registration (deprecated: pieces are found through the hovered tile) ---

    UFUNCTION(BlueprintCallable, Category = "Grid|Hover", meta = (DeprecatedFunction,
        DeprecationMessage = "Pieces aren't hovered any more: find the piece through the hovered tile."))
    void RegisterPiece(AGridPieceBase* Piece);

    UFUNCTION(BlueprintCallable, Category = "Grid|Hover", meta = (DeprecatedFunction,
        DeprecationMessage = "Pieces aren't hovered any more: find the piece through the hovered tile."))
    void UnregisterPiece(AGridPieceBase* Piece);

    UFUNCTION(BlueprintPure, Category = "Grid|Hover")
    bool IsPieceRegistered(const AGridPieceBase* Piece) const;

    // --- Hover Relay ---

    // A registered tile's hover began
    UPROPERTY(BlueprintAssignable, Category = "Grid|Hover")
    FGridHoverSubsystemTileDelegate OnGridTileHoverChanged;

    // A registered tile's hover ended (the cursor left, or the tile was
    // unregistered while hovered)
    UPROPERTY(BlueprintAssignable, Category = "Grid|Hover")
    FGridHoverSubsystemTileDelegate OnGridTileHoverEnded;

    // A registered tile was clicked
    UPROPERTY(BlueprintAssignable, Category = "Grid|Hover")
    FGridHoverSubsystemTileDelegate OnGridTileClicked;

    UPROPERTY(BlueprintAssignable, Category = "Grid|Hover", meta = (DeprecatedProperty,
        DeprecationMessage = "Pieces aren't hovered any more: find the piece through the hovered tile."))
    FGridHoverSubsystemPieceDelegate OnGridPieceHoverChanged;

private:

    UPROPERTY()
    TSet<TObjectPtr<AGridTileBase>> RegisteredTiles;

    UPROPERTY()
    TSet<TObjectPtr<AGridPieceBase>> RegisteredPieces;

    void BindTile(AGridTileBase* Tile, bool bBind);

    UFUNCTION()
    void HandleTileHoverBegin(UGridHoverComponent* Component);

    UFUNCTION()
    void HandleTileHoverEnd(UGridHoverComponent* Component);

    UFUNCTION()
    void HandleTileClicked(UGridHoverComponent* Component);

    UFUNCTION()
    void BroadcastGridPieceHoverChanged(AGridPieceBase* InPiece);

};
