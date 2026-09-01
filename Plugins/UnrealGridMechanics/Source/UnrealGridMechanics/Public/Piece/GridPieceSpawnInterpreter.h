// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GridMechanicsBaseStructs.h"
#include "GridPieceSpawnInterpreter.generated.h"

class AGridPieceBase;
class AGridTileBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGridPieceVisualEvent, AGridPieceBase*, Piece);

// Project-agnostic piece spawn/despawn *visual* executor -- given a piece
// that already exists (pool retrieval, registration, and the
// IActorPoolInterface Activate/Deactivate calls are all
// UGridPieceRegistryComponent's job, not this class's -- see that class),
// this positions it and waits for its own visual completion signal
// (AGridPieceBase::OnActivationVisualComplete/OnDeactivationVisualComplete)
// before reporting done. No tag-watching, no pooling knowledge, no
// position bookkeeping of its own -- deciding WHEN and WHY a piece
// spawns/despawns is entirely a project-specific concern, handled by
// whatever calls SpawnPiece/DespawnPiece (e.g. ConnectIt's own
// ConnectIt_PieceSpawnInterpreter, which watches board-event tags and
// calls into UGridPieceRegistryComponent, which calls in here).
UCLASS(Blueprintable, ClassGroup=(Grid), meta=(BlueprintSpawnableComponent))
class UNREALGRIDMECHANICS_API UGridPieceSpawnInterpreter : public UActorComponent
{
    GENERATED_BODY()

public:

    // Positions Piece at Tile and waits for its spawn-in visual to report
    // complete. Fires OnPieceSpawned once it does. Does not call
    // Execute_ActivatePoolObject -- that already happened as a side effect
    // of retrieving Piece from the pool, before this is ever called.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid|Piece")
    void SpawnPiece(AGridPieceBase* Piece, AGridTileBase* Tile);

    // Waits for Piece's despawn-out visual to report complete. Fires
    // OnPieceDespawned once it does. Does not call
    // Execute_DeactivatePoolObject or touch the pool -- that's the caller's
    // job (see UGridPieceRegistryComponent::DespawnPieceAt), this is purely
    // the visual.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid|Piece")
    void DespawnPiece(AGridPieceBase* Piece);

    UPROPERTY(BlueprintAssignable, Category = "Grid|Piece")
    FOnGridPieceVisualEvent OnPieceSpawned;

    UPROPERTY(BlueprintAssignable, Category = "Grid|Piece")
    FOnGridPieceVisualEvent OnPieceDespawned;

private:

    UFUNCTION()
    void HandleActivationVisualComplete(AGridPieceBase* Piece);

    UFUNCTION()
    void HandleDeactivationVisualComplete(AGridPieceBase* Piece);
};
