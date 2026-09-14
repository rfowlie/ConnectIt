// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridCursorManagerBase.generated.h"

class AGridTileBase;
class UGridHoverSubsystem;

// Abstract contract for a grid cursor -- an actor that tracks whichever
// tile is currently hovered and represents it visually
//
// Binds to UGridHoverSubsystem::OnGridTileHoverChanged -- the same
// hover pipeline UTurnBasedAction uses for hover validation -- so any
// cursor built on this base stays in sync with the networked turn-based
// hover flow automatically, with no extra wiring per project
//
// Subclasses implement UpdateCursor to define the actual visual
// (move a mesh, show a widget, spawn a decal, etc.) -- this base only
// owns the subscription and the enabled/paused state
UCLASS(Abstract, Blueprintable, BlueprintType)
class UNREALGRIDMECHANICS_API AGridCursorManagerBase : public AActor
{
    GENERATED_BODY()

public:

    AGridCursorManagerBase();

    // Stop updating and hide the cursor
    UFUNCTION(BlueprintCallable, Category = "Grid Mechanics|Cursor")
    void EnableCursor(bool bValue);

    // Stop updating but keep the cursor visible in its last position
    UFUNCTION(BlueprintCallable, Category = "Grid Mechanics|Cursor")
    void PauseCursor(bool bValue);

    UFUNCTION(BlueprintPure, Category = "Grid Mechanics|Cursor")
    bool IsCursorEnabled() const { return bIsEnabled; }

protected:

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Called whenever the hovered tile changes -- Tile is null once
    // nothing is hovered. Implement to move/show/hide the cursor visual.
    // Not called while the cursor is paused/disabled
    UFUNCTION(BlueprintNativeEvent, Category = "Grid Mechanics|Cursor")
    void UpdateCursor(AGridTileBase* Tile);
    virtual void UpdateCursor_Implementation(AGridTileBase* Tile);

private:

    UFUNCTION()
    void HandleGridTileHoverChanged(AGridTileBase* Tile);

    UGridHoverSubsystem* GetGridSubsystem() const;

    bool bIsEnabled = true;
};
