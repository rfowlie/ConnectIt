// Fill out your copyright notice in the Description page of Project Settings.

// GridWorldSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GridMechanicsBaseStructs.h"
#include "GridWorldSubsystem.generated.h"


class AGridTileBase;
class AGridPieceBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTileRegistered,
    AGridTileBase*, Tile, FGridPosition, Position);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTileUnregistered,
    AGridTileBase*, Tile, FGridPosition, Position);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPieceRegistered,
    AGridPieceBase*, Piece, FGridPosition, Position);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPieceUnregistered,
    AGridPieceBase*, Piece, FGridPosition, Position);

UCLASS()
class UNREALGRIDMECHANICS_API UGridWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:

    // --- Tile Registration ---

    UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
    void RegisterTile(AGridTileBase* Tile, FGridPosition Position);

    UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
    void UnregisterTile(AGridTileBase* Tile);

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    AGridTileBase* GetTileAtPosition(FGridPosition Position) const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    bool IsTileRegistered(AGridTileBase* Tile) const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    TArray<AGridTileBase*> GetAllTiles() const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    TArray<FGridPosition> GetAllTilePositions() const;

    // --- Piece Registration ---

    UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
    void RegisterPiece(AGridPieceBase* Piece, FGridPosition Position);

    UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
    void UnregisterPiece(AGridPieceBase* Piece);

    UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
    void UpdatePiecePosition(AGridPieceBase* Piece, FGridPosition NewPosition);

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    AGridPieceBase* GetPieceAtPosition(FGridPosition Position) const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    bool IsPieceRegistered(AGridPieceBase* Piece) const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    TArray<AGridPieceBase*> GetAllPieces() const;

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Grid|Registry")
    FOnTileRegistered OnTileRegistered;

    UPROPERTY(BlueprintAssignable, Category = "Grid|Registry")
    FOnTileUnregistered OnTileUnregistered;

    UPROPERTY(BlueprintAssignable, Category = "Grid|Registry")
    FOnPieceRegistered OnPieceRegistered;

    UPROPERTY(BlueprintAssignable, Category = "Grid|Registry")
    FOnPieceUnregistered OnPieceUnregistered;

private:

    // Tile maps — bidirectional for fast lookup in both directions
    UPROPERTY()
    TMap<FGridPosition, AGridTileBase*> TilesByPosition;

    UPROPERTY()
    TMap<AGridTileBase*, FGridPosition> PositionsByTile;

    // Piece maps — bidirectional
    UPROPERTY()
    TMap<FGridPosition, AGridPieceBase*> PiecesByPosition;

    UPROPERTY()
    TMap<AGridPieceBase*, FGridPosition> PositionsByPiece;
};