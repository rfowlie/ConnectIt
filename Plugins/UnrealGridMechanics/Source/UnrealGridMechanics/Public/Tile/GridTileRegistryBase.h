// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "UObject/Object.h"
#include "GridTileRegistryBase.generated.h"

class AGridTileBase;
class UGridHoverSubsystem;

// UObject-based prototype counterpart to UGridTileRegistryComponent -- same
// public query surface and the same responsibilities: it owns the
// authoritative tile list, discovers tiles (level-placed + spawned later),
// registers each with UGridHoverSubsystem so hover can be relayed, and
// answers grid queries against that list. Not an ActorComponent -- it has
// no engine lifecycle of its own, so ABoardManagerBase drives it via
// InitialiseRegistry / ShutdownRegistry from its own BeginPlay / EndPlay.
// Meant to be held as an EditAnywhere, Instanced property (e.g. on
// ABoardManagerBase) -- EditInlineNew is what makes that render as a real
// class-picker with inline-editable sub-properties in the Details panel,
// true BP-only runtime polymorphism with no C++ subclass required.
//
// GridPositionToWorld/WorldToGridPosition are the one genuine per-subclass
// override point (BlueprintNativeEvent) -- a project with a non-square grid
// (hex, isometric) would only need to override these two; every other
// query here is derived from them plus the owned tile list, matching
// UGridPieceRegistryComponent::InstantiatePiece's existing precedent of
// "one clearly-marked hook, not everything virtual."
// Not Abstract -- everything here is either fully implemented already or an
// optional override point (GridPositionToWorld/WorldToGridPosition), so
// this is directly usable with no subclass required, and can be defaulted
// via CreateDefaultSubobject from an owning Actor's constructor -- the
// standard UE idiom for giving an Instanced UObject property a
// constructor-time default that still participates correctly in CDO/
// archetype propagation and Blueprint-child override.
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class UNREALGRIDMECHANICS_API UGridTileRegistryBase : public UObject
{
    GENERATED_BODY()

public:

    // Size of each grid cell in world units -- all default position
    // conversions use this value.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid|Registry")
    int32 GridSize = 200;

    // --- Position Conversion (override point) ---

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Grid|Registry")
    FVector GridPositionToWorld(FGridPosition Position) const;
    virtual FVector GridPositionToWorld_Implementation(FGridPosition Position) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Grid|Registry")
    FGridPosition WorldToGridPosition(const FVector& WorldLocation) const;
    virtual FGridPosition WorldToGridPosition_Implementation(const FVector& WorldLocation) const;

    // --- Tile Queries ---

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    AGridTileBase* GetTileAtPosition(FGridPosition Position) const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    FGridPosition GetPositionOfTile(const AGridTileBase* Tile) const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    TArray<AGridTileBase*> GetRow(int32 RowIndex) const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    TArray<AGridTileBase*> GetColumn(int32 ColumnIndex) const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    TArray<FGridPosition> GetAllTilePositions() const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    TArray<AGridTileBase*> GetAllTiles() const;

    // --- Board Dimension Queries ---
    // Derived from registered tile positions -- no hardcoded dimensions

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

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    TArray<FGridPosition> GetRowPositions(int32 RowIndex) const;

    UFUNCTION(BlueprintPure, Category = "Grid|Registry")
    TArray<FGridPosition> GetColumnPositions(int32 ColumnIndex) const;

    // --- Lifecycle (driven by the owning ABoardManagerBase) ---

    // Discovers every AGridTileBase in the world, takes ownership of the
    // list, and registers each with UGridHoverSubsystem for hover relay.
    // Also starts listening for tiles spawned / streamed in later.
    virtual void InitialiseRegistry();

    // Unregisters every owned tile and stops listening for new spawns.
    virtual void ShutdownRegistry();

private:

    // Authoritative, ordered list of every tile in the level.
    UPROPERTY()
    TArray<TObjectPtr<AGridTileBase>> Tiles;

    // Cached hover subsystem -- used only to register / unregister tiles.
    // A properly Outer'd UObject (NewObject<T>(OwningActor, ...)) gets a
    // working GetWorld() for free via the Outer chain.
    UGridHoverSubsystem* ResolveHoverSubsystem();

    UPROPERTY()
    TObjectPtr<UGridHoverSubsystem> HoverSubsystem = nullptr;

    void DiscoverTiles();
    void HandleActorSpawned(AActor* SpawnedActor);
    FDelegateHandle ActorSpawnedHandle;
};
