// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "UObject/Object.h"
#include "GridDefinition.generated.h"

// Grid geometry -- cell size and the World<->Grid position conversion --
// extracted out of UGridTileRegistryBase, which used to own this alongside
// tile-actor tracking. The two are different concerns: geometry answers
// "where in the world is grid position (X,Y)", tile tracking answers
// "which actor currently occupies it". Bundling them meant a piece
// registry that only needed the geometry had to depend on a whole tile
// registry to reach it (UGridPieceRegistryBase::TileRegistry existed for
// exactly this, and nothing else). Both registries now hold a
// UGridDefinition reference as siblings instead.
//
// GridPositionToWorld/WorldToGridPosition are the one genuine per-subclass
// override point (BlueprintNativeEvent) -- a project with a non-square grid
// (hex, isometric) only needs to override these two. This was previously
// true of UGridTileRegistryBase; the override point moves here with the
// rest of the geometry.
//
// Meant to be held as an EditAnywhere, Instanced property (e.g. on a level
// config data asset), same pattern as UGridTileRegistryBase/
// UGridPieceRegistryBase -- EditInlineNew is what makes that render as a
// real class-picker with inline-editable sub-properties in the Details
// panel. Not Abstract -- fully usable with no subclass required.
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class UNREALGRIDMECHANICS_API UGridDefinition : public UObject
{
    GENERATED_BODY()

public:

    // Size of each grid cell in world units -- all default position
    // conversions use this value.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid|Definition")
    int32 GridSize = 200;

    // --- Position Conversion (override point) ---

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Grid|Definition")
    FVector GridPositionToWorld(FGridPosition Position) const;
    virtual FVector GridPositionToWorld_Implementation(FGridPosition Position) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Grid|Definition")
    FGridPosition WorldToGridPosition(const FVector& WorldLocation) const;
    virtual FGridPosition WorldToGridPosition_Implementation(const FVector& WorldLocation) const;
};
