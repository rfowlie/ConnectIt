// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GridPieceSpawnInterpreterBase.generated.h"

class AGridPieceBase;
class AGridTileBase;

// Named distinctly from UGridPieceSpawnInterpreter's own FOnGridPieceVisualEvent
// (same module, same signature) -- two DECLARE_DYNAMIC_MULTICAST_DELEGATE
// macros expand to global-scope classes, so reusing that name here would be
// a duplicate-symbol compile error, not just a style clash.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGridPieceVisualEventBase, AGridPieceBase*, Piece);

// UObject-based prototype counterpart to UGridPieceSpawnInterpreter -- same
// public surface (SpawnPiece/DespawnPiece, OnPieceSpawned/OnPieceDespawned),
// but not an ActorComponent, since it was already purely delegate-driven
// (no Tick usage at all). Meant to be held as an EditAnywhere, Instanced
// property for true BP-only runtime polymorphism -- see
// UGridTileRegistryBase's class comment for the full reasoning, identical
// here. No logic changes from the live component -- positions a piece and
// waits for its own visual-completion signal before reporting done.
// Not Abstract -- see UGridTileRegistryBase's class comment for why
// (SpawnPiece/DespawnPiece are already fully implemented, no subclass
// required for the common case).
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class UNREALGRIDMECHANICS_API UGridPieceSpawnInterpreterBase : public UObject
{
    GENERATED_BODY()

public:

    // Positions Piece at Tile and waits for its spawn-in visual to report
    // complete. Fires OnPieceSpawned once it does. Does not call
    // Execute_ActivatePoolObject -- that's the caller's job (see
    // UGridPieceRegistryBase::ActivatePieceAt), triggered separately.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid|Piece")
    void SpawnPiece(AGridPieceBase* Piece, AGridTileBase* Tile);
    virtual void SpawnPiece_Implementation(AGridPieceBase* Piece, AGridTileBase* Tile);

    // Waits for Piece's despawn-out visual to report complete. Fires
    // OnPieceDespawned once it does. Does not call
    // Execute_DeactivatePoolObject or touch the pool -- purely the visual.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid|Piece")
    void DespawnPiece(AGridPieceBase* Piece);
    virtual void DespawnPiece_Implementation(AGridPieceBase* Piece);

    UPROPERTY(BlueprintAssignable, Category = "Grid|Piece")
    FOnGridPieceVisualEventBase OnPieceSpawned;

    UPROPERTY(BlueprintAssignable, Category = "Grid|Piece")
    FOnGridPieceVisualEventBase OnPieceDespawned;

private:

    UFUNCTION()
    void HandleActivationVisualComplete(AGridPieceBase* Piece);

    UFUNCTION()
    void HandleDeactivationVisualComplete(AGridPieceBase* Piece);
};
