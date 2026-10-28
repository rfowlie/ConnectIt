// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interface/GridFactionInterface.h"
#include "Interface/GridLevelInterface.h"
#include "Interface/GridTileHandler.h"
#include "Piece/GridPieceBase.h"
#include "Pooling/ActorPoolInterface.h"
#include "ConnectIt_PieceBase.generated.h"

/*
 * base class for blueprints to ensure we are adding all necessary interfaces
 *
 * A piece actor is presentation, local to each machine, like a tile actor:
 * every machine takes its own from its actor pool when a board event plays
 * (the piece registry's Spawn Piece At). It doesn't replicate -- a replicated
 * piece the server pooled would show up on clients beside their own.
 */
UCLASS(BlueprintType, Blueprintable)
class CONNECTIT_API AConnectIt_PieceBase : public AGridPieceBase,
	public IGridFactionInterface,
	public IGridLevelInterface,
	public IGridTileHandler
{
	GENERATED_BODY()

public:
	AConnectIt_PieceBase();

	// Set faction and trigger visual update
	// Called immediately after retrieval from pool
	UFUNCTION(BlueprintCallable, Category = "ConnectIt|Piece")
	void InitialisePiece(int32 InFactionID, FGridPosition InGridPosition);

	

	// Grid position this piece occupies
	UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Piece")
	FGridPosition OccupiedPosition;

	// IGridBoardActor: takes its position from the piece registry and its
	// faction from the display board there; a changed faction updates the
	// visuals (OnFactionVisualUpdate)
	virtual void RefreshFromBoard_Implementation() override;

	// IActorPoolInterface
	// Blueprint implements the actual visual setup/teardown
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ActivatePoolObject();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void DeactivatePoolObject();

protected:

	// Faction this piece belongs to
	UPROPERTY(BlueprintReadWrite, Category = "ConnectIt|Piece")
	int32 FactionID = -1;

	// TODO: rename to Initialize_Internal as this fires during the initialize function
	// Called by InitialisePiece
	// Blueprint override to update mesh, material, particle etc.
	UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Piece")
	void OnFactionVisualUpdate(int32 InFactionID);
	virtual void OnFactionVisualUpdate_Implementation(int32 InFactionID);

};
