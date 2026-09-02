// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "Components/ActorComponent.h"
#include "GridPieceRegistryComponent.generated.h"

class AGridPieceBase;
class AGridTileBase;
class UGridTileRegistryComponent;
class UGridHoverSubsystem;
class UGridPieceSpawnInterpreter;

// Owns piece existence end-to-end: answers "what piece is at this
// position" (GetPiece), and owns getting one there or taking one away --
// pool retrieval/release (UActorPoolSubsystem), hover-subsystem
// registration/unregistration (UGridHoverSubsystem::RegisterPiece/
// UnregisterPiece), and triggering the IActorPoolInterface Activate/
// Deactivate calls (UActorPoolSubsystem::ActivateObject/DeactivateObject)
// all live here. The actual spawn-in/despawn-out *visual* is a sibling
// component's job (UGridPieceSpawnInterpreter, resolved via the owning
// actor) -- this class hands it an already-retrieved, already-registered
// piece and lets it handle positioning and waiting for the visual to
// finish.
//
// SpawnPieceAt/DespawnPieceAt are convenience entry points composed from
// four lower-level primitives (RetrievePiece/ActivatePieceAt,
// DeactivatePiece/ReleasePiece) -- a caller that needs to inject work
// between steps (e.g. project-specific initialization between retrieval
// and the visual, so the visual reflects it when it plays; or gating
// finalization on visual completion instead of triggering and finalizing
// in the same breath) calls the primitives directly instead. See
// UConnectIt_PlacePieceGameEvent/UConnectIt_LineScoreGameEvent for the
// reference callers of each. SpawnPieceAt/DespawnPieceAt still trigger
// activation/deactivation only after UGridPieceSpawnInterpreter has bound
// its completion listener -- a piece whose activation/deactivation
// completes synchronously would otherwise have that completion missed.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNREALGRIDMECHANICS_API UGridPieceRegistryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGridPieceRegistryComponent();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Grid|Registry")
	AGridPieceBase* InstantiatePiece(FGridPosition Position);
	
	UFUNCTION(BlueprintPure, Category = "Grid|Registry")
	AGridPieceBase* GetPiece(const FGridPosition Position);

	// Retrieves a PieceClass instance from the pool (inactive at this
	// point), registers it with UGridHoverSubsystem, hands it to
	// UGridPieceSpawnInterpreter to position at Tile and bind its spawn-in
	// visual-completion listener, and only then triggers
	// IActorPoolInterface::ActivatePoolObject (via UActorPoolSubsystem::
	// ActivateObject) -- deliberately after the listener is bound, so a
	// piece whose activation completes synchronously can't have that
	// completion missed. Returns the piece, or nullptr if retrieval or
	// either sibling component failed to resolve. Convenience composition
	// of RetrievePiece + ActivatePieceAt -- see those for callers that
	// need to do something in between (e.g. initialization).
	UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
	AGridPieceBase* SpawnPieceAt(TSubclassOf<AGridPieceBase> PieceClass, AGridTileBase* Tile);

	// Looks up the piece at Position (via GetPiece), triggers its
	// deactivation, and immediately finalizes removal (unregister +
	// release to pool) -- no gap between trigger and finalize. No-ops
	// (with a warning) if no piece is registered at Position. Convenience
	// composition of DeactivatePiece + ReleasePiece -- see those for a
	// caller that needs to gate finalization on visual completion instead.
	UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
	void DespawnPieceAt(FGridPosition Position);

	// Pool retrieval + UGridHoverSubsystem registration only -- does NOT
	// position or activate the piece. Split out so a caller can inject
	// work (e.g. project-specific initialization) between retrieval and
	// the visual, which needs to happen before the visual so it reflects
	// the initialized state when it plays. Returns nullptr (logged) on
	// failure.
	UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
	AGridPieceBase* RetrievePiece(TSubclassOf<AGridPieceBase> PieceClass);

	// Positions Piece at Tile (via UGridPieceSpawnInterpreter::SpawnPiece,
	// which also binds the visual-completion listener) and triggers
	// activation -- deliberately after that bind, same reasoning
	// SpawnPieceAt already documents.
	UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
	void ActivatePieceAt(AGridPieceBase* Piece, AGridTileBase* Tile);

	// Triggers Piece's pooled-deactivation side effect. Does NOT unregister
	// or release it -- call ReleasePiece once it's safe to finalize the
	// removal (e.g. once whatever visual-completion signal you bound
	// before calling this has fired). This split is what lets a caller
	// gate "Position stops mapping to Piece" on visual completion instead
	// of finalizing removal in the same breath as triggering it.
	UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
	void DeactivatePiece(AGridPieceBase* Piece) const;

	// Finalizes removal: unregisters from UGridHoverSubsystem, releases to
	// the pool. Call only once you're actually done with Piece.
	UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
	void ReleasePiece(AGridPieceBase* Piece);

	// --- Position Conversion ---

	UFUNCTION(BlueprintPure, Category = "Grid|Registry")
	FVector GridPositionToWorld(FGridPosition Position) const;

	UFUNCTION(BlueprintPure, Category = "Grid|Registry")
	FGridPosition WorldToGridPosition(const FVector& WorldLocation) const;

	// --- Queries ---

	UFUNCTION(BlueprintPure, Category = "Grid|Registry")
	AGridTileBase* GetPieceAtPosition(FGridPosition Position) const;
    
	UFUNCTION(BlueprintPure, Category = "Grid|Registry")
	FGridPosition GetPositionOfPiece(const AGridPieceBase* Tile) const;

	// UFUNCTION(BlueprintPure, Category = "Grid|Registry")
	// TArray<AGridPieceBase*> GetRow(int32 RowIndex) const;
	//
	// UFUNCTION(BlueprintPure, Category = "Grid|Registry")
	// TArray<AGridPieceBase*> GetColumn(int32 ColumnIndex) const;

	UFUNCTION(BlueprintPure, Category = "Grid|Registry")
	TArray<FGridPosition> GetAllPositions() const;
	
	UFUNCTION(BlueprintPure, Category = "Grid|Registry")
	TArray<FGridPosition> GetAllPositionsOfFaction(const int32 FactionId) const;

	UFUNCTION(BlueprintPure, Category = "Grid|Registry")
	TArray<AGridPieceBase*> GetAllPiecesOfFaction(const int32 FactionId) const;
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadOnly)
	TMap<FGridPosition, AGridPieceBase*> PieceMap;

private:
	
	UPROPERTY()
	TObjectPtr<UGridTileRegistryComponent> GridTileRegistryComponent = nullptr;

	// UPROPERTY()
	// TObjectPtr<UGridPieceSpawnInterpreter> SpawnInterpreter = nullptr;

	// Resolves and caches the subsystem/component references
	bool ResolveTileRegistry();

};
