// Fill out your copyright notice in the Description page of Project Settings.

// GridRegistryComponent.h
#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "Components/ActorComponent.h"
#include "GridTileRegistryComponent.generated.h"

class AGridTileBase;
class UGridWorldSubsystem;

UCLASS(ClassGroup=(Grid), meta=(BlueprintSpawnableComponent))
class UNREALGRIDMECHANICS_API UGridTileRegistryComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UGridTileRegistryComponent();

    // Size of each grid cell in world units
    // All position conversions use this value
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid|Registry")
    int32 GridSize = 200;

    // --- Position Conversion ---

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    FVector GridPositionToWorld(FGridPosition Position) const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    FGridPosition WorldToGridPosition(const FVector& WorldLocation) const;

    // --- Tile Queries ---

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    AGridTileBase* GetTile(FGridPosition Position) const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    TArray<AGridTileBase*> GetRow(int32 RowIndex) const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    TArray<AGridTileBase*> GetColumn(int32 ColumnIndex) const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    TArray<FGridPosition> GetAllTilePositions() const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    TArray<AGridTileBase*> GetAllTiles() const;

    // --- Board Dimension Queries ---
    // Derived from registered tile positions — no hardcoded dimensions

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    int32 GetMinRow() const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    int32 GetMaxRow() const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    int32 GetMinColumn() const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    int32 GetMaxColumn() const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    int32 GetRowCount() const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    int32 GetColumnCount() const;

    // Returns all positions in a given row
    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    TArray<FGridPosition> GetRowPositions(int32 RowIndex) const;

    // Returns all positions in a given column
    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    TArray<FGridPosition> GetColumnPositions(int32 ColumnIndex) const;

protected:

    virtual void BeginPlay() override;

private:
    // Cached subsystem reference — resolved once on BeginPlay
    UPROPERTY()
    UGridWorldSubsystem* GridSubsystem = nullptr;

    // Resolves and caches the subsystem reference
    bool ResolveSubsystem();

    //-----------------EDITOR----------------------
#if WITH_EDITOR
    // Validates tile world positions against GridSize in editor builds
    // Logs warnings for misaligned tiles
    void ValidateTileAlignment() const;
    
#endif
};
