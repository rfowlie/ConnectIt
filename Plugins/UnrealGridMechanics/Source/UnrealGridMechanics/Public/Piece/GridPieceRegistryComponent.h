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
// (UGridWorldSubsystem::RegisterPiece/UnregisterPiece), and the
// IActorPoolInterface Activate/Deactivate calls all live here. The actual
// spawn-in/despawn-out *visual* is a sibling component's job
// (UGridPieceSpawnInterpreter, resolved via the owning actor) -- this
// class hands it an already-retrieved, already-registered piece and lets
// it handle positioning and waiting for the visual to finish.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNREALGRIDMECHANICS_API UGridPieceRegistryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGridPieceRegistryComponent();

	UFUNCTION(BlueprintPure, Category = "Grid|Registry")
	AGridPieceBase* GetPiece(FGridPosition Position) const;

	// Retrieves a PieceClass instance from the pool, registers it with
	// UGridWorldSubsystem, and hands it to UGridPieceSpawnInterpreter to
	// position at Tile and play its spawn-in visual. Retrieval itself
	// triggers IActorPoolInterface::ActivatePoolObject as a side effect
	// (see UActorPool) -- this function does not call it separately.
	// Returns the piece, or nullptr if retrieval or either sibling
	// component failed to resolve.
	UFUNCTION(BlueprintCallable, Category = "Grid|Registry")
	AGridPieceBase* SpawnPieceAt(TSubclassOf<AGridPieceBase> PieceClass, AGridTileBase* Tile);

	// Looks up the piece at Position (via GetPiece), unregisters it from
	// UGridWorldSubsystem, releases it back to the pool (which triggers
	// IActorPoolInterface::DeactivatePoolObject as a side effect), and
	// hands it to UGridPieceSpawnInterpreter to play its despawn-out
	// visual. No-ops (with a warning) if no piece is registered at
	// Position.
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
