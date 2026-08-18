// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "Board/Interpreter/BoardStateInterpreter.h"
#include "Pooling/ActorPool.h"
#include "ConnectIt_PieceSpawnInterpreter.generated.h"

class AConnectIt_GridPiece;


// Spawns and despawns client-local piece actors based on board state changes
// Piece actors are CLIENT LOCAL -- never replicated, never exist on server
// Each client manages its own pool independently
// Runs on ALL clients independently
UCLASS(Blueprintable, ClassGroup=(ConnectIt),
	meta=(BlueprintSpawnableComponent))
class CONNECTIT_API UConnectIt_PieceSpawnInterpreter : public UBoardStateInterpreter
{
	GENERATED_BODY()

public:

	// Piece actor class -- set in editor or from config
	// Must be AConnectItGridPiece subclass
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interpreter|Pool")
	TSubclassOf<AConnectIt_GridPiece> PieceActorClass = nullptr;

	// How many pieces to pre-warm in the client pool
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interpreter|Pool",
		meta = (ClampMin = 1))
	int32 PoolInitialSize = 50;

protected:

	virtual void BeginPlay() override;

	virtual void OnBoardStateChanged_Implementation(
		const UBoardStateComponentBase* Component) override;

private:

	// Client local pool -- not replicated, not shared with server
	UPROPERTY()
	TObjectPtr<UActorPool> PiecePool = nullptr;

	// Active pieces keyed by grid position -- client local
	TMap<FGridPosition, TObjectPtr<AConnectIt_GridPiece>> ActivePieces;

	void SpawnPieceAt(
		const FGridPosition& Position,
		int32 FactionID);

	void ReturnPieceToPool(const FGridPosition& Position);

	FVector GetWorldPositionForTile(const FGridPosition& Position) const;
};