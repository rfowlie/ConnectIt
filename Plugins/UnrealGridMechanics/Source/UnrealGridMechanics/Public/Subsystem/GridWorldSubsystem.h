// Fill out your copyright notice in the Description page of Project Settings.

// GridWorldSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GridMechanicsBaseStructs.h"
#include "GridWorldSubsystem.generated.h"


class AGridTileBase;
class AGridPieceBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridWorldSubsystemTileDelegate, AGridTileBase*, Tile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridWorldSubsystemPieceDelegate, AGridPieceBase*, Piece);

/*
 * keep track of all grid pieces in the current level
 * provide global point for binding cursor overlaps
 */
UCLASS()
class UNREALGRIDMECHANICS_API UGridWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:

    // --- Tile Registration ---

    UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
    void RegisterTile(AGridTileBase* Tile);

    UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
    void UnregisterTile(AGridTileBase* Tile);

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    bool IsTileRegistered(const AGridTileBase* Tile) const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    TArray<AGridTileBase*> GetAllTiles() const;
    

    // --- Piece Registration ---

    UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
    void RegisterPiece(AGridPieceBase* Piece);

    UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
    void UnregisterPiece(AGridPieceBase* Piece);

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    bool IsPieceRegistered(const AGridPieceBase* Piece) const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    TArray<AGridPieceBase*> GetAllPieces() const;

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Grid|Registry")
    FGridWorldSubsystemTileDelegate OnTileRegistered;

    UPROPERTY(BlueprintAssignable, Category = "Grid|Registry")
    FGridWorldSubsystemTileDelegate OnTileUnregistered;

    UPROPERTY(BlueprintAssignable, Category = "Grid|Registry")
    FGridWorldSubsystemPieceDelegate OnPieceRegistered;

    UPROPERTY(BlueprintAssignable, Category = "Grid|Registry")
    FGridWorldSubsystemPieceDelegate OnPieceUnregistered;

    UPROPERTY(BlueprintAssignable, Category = "Grid|Registry")
    FGridWorldSubsystemTileDelegate OnGridTileHoverChanged;
    
private:

    // Tile maps — bidirectional for fast lookup in both directions

    UPROPERTY()
    TMap<AGridTileBase*, bool> RegisteredTiles;

    UPROPERTY()
    TArray<AGridPieceBase*> RegisteredPieces;

    UFUNCTION()
    void BroadcastGridTileHoverChanged(AGridTileBase* InTile);
    
};