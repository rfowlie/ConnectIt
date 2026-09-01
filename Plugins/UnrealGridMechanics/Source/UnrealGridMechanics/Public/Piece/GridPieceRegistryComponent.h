// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "Components/ActorComponent.h"
#include "GridPieceRegistryComponent.generated.h"

class AGridPieceBase;
class AGridTileBase;
class UGridTileRegistryComponent;
class UGridWorldSubsystem;
class UGridPieceSpawnInterpreter;

// Owns piece existence end-to-end: answers "what piece is at this
// position" (GetPiece), and owns getting one there or taking one away
// (SpawnPieceAt/DespawnPieceAt) -- pool retrieval/release
// (UActorPoolSubsystem), grid-subsystem registration/unregistration
// (UGridWorldSubsystem::RegisterPiece/UnregisterPiece), and triggering the
// IActorPoolInterface Activate/Deactivate calls (UActorPoolSubsystem::
// ActivateObject/DeactivateObject) all live here. The actual spawn-in/
// despawn-out *visual* is a sibling component's job (UGridPieceSpawnInterpreter,
// resolved via the owning actor) -- this class hands it an already-retrieved,
// already-registered piece and lets it handle positioning and waiting for
// the visual to finish, and deliberately triggers activation/deactivation
// only after that sibling has bound its completion listener (see
// SpawnPieceAt/DespawnPieceAt) -- a piece whose activation/deactivation
// completes synchronously would otherwise have that completion missed.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNREALGRIDMECHANICS_API UGridPieceRegistryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGridPieceRegistryComponent();

	UFUNCTION(BlueprintPure, Category = "Grid|Registry")
	AGridPieceBase* GetPiece(FGridPosition Position) const;

	// Retrieves a PieceClass instance from the pool (inactive at this
	// point), registers it with UGridWorldSubsystem, hands it to
	// UGridPieceSpawnInterpreter to position at Tile and bind its spawn-in
	// visual-completion listener, and only then triggers
	// IActorPoolInterface::ActivatePoolObject (via UActorPoolSubsystem::
	// ActivateObject) -- deliberately after the listener is bound, so a
	// piece whose activation completes synchronously can't have that
	// completion missed. Returns the piece, or nullptr if retrieval or
	// either sibling component failed to resolve.
	UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
	AGridPieceBase* SpawnPieceAt(TSubclassOf<AGridPieceBase> PieceClass, AGridTileBase* Tile);

	// Looks up the piece at Position (via GetPiece), unregisters it from
	// UGridWorldSubsystem, hands it to UGridPieceSpawnInterpreter to bind
	// its despawn-out visual-completion listener, then triggers
	// IActorPoolInterface::DeactivatePoolObject and releases it back to
	// the pool -- same bind-before-trigger ordering as SpawnPieceAt, and
	// for the same reason. No-ops (with a warning) if no piece is
	// registered at Position.
	UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
	void DespawnPieceAt(FGridPosition Position);

protected:
	virtual void BeginPlay() override;

private:
	// Cached subsystem/sibling-component references — resolved once on BeginPlay
	UPROPERTY()
	TObjectPtr<UGridWorldSubsystem> GridSubsystem = nullptr;

	UPROPERTY()
	TObjectPtr<UGridTileRegistryComponent> GridTileRegistryComponent = nullptr;

	UPROPERTY()
	TObjectPtr<UGridPieceSpawnInterpreter> SpawnInterpreter = nullptr;

	// Resolves and caches the subsystem/component references
	bool ResolveSubsystem();
	bool ResolveTileRegistry();
	bool ResolveSpawnInterpreter();
};
