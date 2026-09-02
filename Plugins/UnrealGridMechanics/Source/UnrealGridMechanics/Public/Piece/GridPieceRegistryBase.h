// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "UObject/Object.h"
#include "GridPieceRegistryBase.generated.h"

class UGridHoverSubsystem;
class AGridPieceBase;
class AGridTileBase;
class UGridTileRegistryBase;

// UObject-based prototype counterpart to UGridPieceRegistryComponent -- same
// public surface (GetPiece/RetrievePiece/ActivatePieceAt/DeactivatePiece/
// ReleasePiece/SpawnPieceAt/DespawnPieceAt/InstantiatePiece), but not an
// ActorComponent, for the same reasoning as UGridTileRegistryBase (never
// ticked, purely reactive/query -- see that class's comment).
//
// Explicit dependency injection (Initialise) replaces the live component's
// GetOwner()->FindComponentByClass<T>() sibling lookup -- a plain UObject
// isn't attached to an Actor's component list, so that call isn't
// available here regardless; this also matches the pattern this session
// already settled on for UTurnBasedGameEvent subclasses
// (Initialise(PieceRegistry, SpawnInterpreter, ...)).
// Not Abstract -- see UGridTileRegistryBase's class comment for why (same
// reasoning applies: InstantiatePiece is an optional BlueprintImplementableEvent,
// not a requirement to subclass, and everything else is already fully
// implemented).
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class UNREALGRIDMECHANICS_API UGridPieceRegistryBase : public UObject
{
    GENERATED_BODY()

public:

    // Called once by whoever owns this instance (e.g. ABoardManagerBase),
    // before any other method here is used.
    // Discovers every AGridTileBase in the world, takes ownership of the
    // list, and registers each with UGridHoverSubsystem for hover relay.
    // Also starts listening for tiles spawned / streamed in later.
    virtual void InitialiseRegistry();

    // Get-or-create: returns the piece already mapped to Position, or asks
    // InstantiatePiece to create one and caches the result.
    UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
    AGridPieceBase* GetPiece(FGridPosition Position);

    // Designer-implemented creation hook -- called by GetPiece the first
    // time a position is queried with nothing mapped yet.
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Grid|Registry")
    AGridPieceBase* InstantiatePiece(FGridPosition Position);

    // Retrieves a PieceClass instance from the pool (inactive at this
    // point) and registers it with UGridHoverSubsystem. Does NOT position
    // or activate it -- see ActivatePieceAt. Split out so a caller can
    // inject work (e.g. project-specific initialization) between
    // retrieval and the visual, which needs to happen before the visual
    // so it reflects the initialized state when it plays.
    UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
    AGridPieceBase* RetrievePiece(TSubclassOf<AGridPieceBase> PieceClass);

    // Positions Piece at Tile (via SpawnInterpreter, which also binds the
    // visual-completion listener) and triggers activation -- deliberately
    // after that bind, so a piece whose activation completes synchronously
    // can't have that completion missed.
    UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
    void ActivatePieceAt(AGridPieceBase* Piece, AGridTileBase* Tile);

    // Triggers Piece's pooled-deactivation side effect. Does NOT unregister
    // or release it -- call ReleasePiece once it's safe to finalize the
    // removal (e.g. once whatever visual-completion signal you bound
    // before calling this has fired).
    UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
    void DeactivatePiece(AGridPieceBase* Piece) const;

    // Finalizes removal: unregisters from UGridHoverSubsystem, releases to
    // the pool. Call only once you're actually done with Piece.
    UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
    void ReleasePiece(AGridPieceBase* Piece);

    // Convenience composition of RetrievePiece + ActivatePieceAt.
    UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
    AGridPieceBase* SpawnPieceAt(TSubclassOf<AGridPieceBase> PieceClass, AGridTileBase* Tile);

    // Convenience composition of DeactivatePiece + ReleasePiece -- no gap
    // between trigger and finalize. A caller that needs to gate
    // finalization on visual completion instead calls the two primitives
    // directly rather than this.
    UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
    void DespawnPieceAt(FGridPosition Position);

protected:
    
    UPROPERTY(BlueprintReadOnly)
    TMap<FGridPosition, TObjectPtr<AGridPieceBase>> PieceMap;

    UPROPERTY()
    TObjectPtr<UGridTileRegistryBase> TileRegistry = nullptr;

private:

    UGridHoverSubsystem* ResolveHoverSubsystem() const;
};
