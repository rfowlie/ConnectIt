// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseEnums.h"
#include "GridMechanicsBaseStructs.h"
#include "UObject/Object.h"
#include "GridTileRegistryBase.generated.h"

class AGridTileBase;
class UGridHoverSubsystem;
class UGridDefinition;

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
// Grid geometry (cell size, World<->Grid conversion) lives on GridDefinition
// now, not here -- a different concern from tracking which tile actor
// occupies which position. Assigned by whatever constructs this registry
// (see UConnectIt_BoardRegistrySubsystem::OnWorldBeginPlay for the project's
// wiring); every query here that needs a position still goes through it.
// Not Abstract -- fully usable with no subclass required, and can be
// defaulted via CreateDefaultSubobject from an owning Actor's constructor --
// the standard UE idiom for giving an Instanced UObject property a
// constructor-time default that still participates correctly in CDO/
// archetype propagation and Blueprint-child override.
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class UNREALGRIDMECHANICS_API UGridTileRegistryBase : public UObject
{
    GENERATED_BODY()

public:

    // Grid geometry this registry's queries convert positions through.
    // Assigned by whatever constructs this registry -- see the class
    // comment above.
    UPROPERTY()
    TObjectPtr<UGridDefinition> GridDefinition;

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
    TArray<AGridTileBase*> GetTilesByDirection(FGridPosition StartPosition, EGridDirection GridDirection) const;

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
    UFUNCTION(BlueprintNativeEvent, Category = "Grid|Registry")
    void InitialiseRegistry();

    // Unregisters every owned tile and stops listening for new spawns.
    UFUNCTION(BlueprintNativeEvent, Category = "Grid|Registry")
    void ShutdownRegistry();

protected:

    // TODO: deprecate this
    // Authoritative, ordered list of every tile in the level.
    UPROPERTY(BlueprintReadWrite)
    TArray<TObjectPtr<AGridTileBase>> Tiles;

    UPROPERTY(BlueprintReadWrite, Category = "Grid|Registry")
    TMap<FGridPosition, TObjectPtr<AGridTileBase>> TileMap;

    UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
    void UpdateMappings();
    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid|Registry")
    void DiscoverExisting();
    
    void HandleActorSpawned(AActor* SpawnedActor);
    
    FDelegateHandle ActorSpawnedHandle;


    // --- SubSystem ---
    
    // Cached hover subsystem -- used only to register / unregister tiles.
    // A properly Outer'd UObject (NewObject<T>(OwningActor, ...)) gets a
    // working GetWorld() for free via the Outer chain.
    UGridHoverSubsystem* ResolveHoverSubsystem();

    UPROPERTY()
    TObjectPtr<UGridHoverSubsystem> HoverSubsystem = nullptr;

    
};
