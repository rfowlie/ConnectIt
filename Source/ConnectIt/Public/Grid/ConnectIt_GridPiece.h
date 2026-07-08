// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interface/GridFactionInterface.h"
#include "Interface/GridLevelInterface.h"
#include "Interface/GridTileHandler.h"
#include "Piece/GridPieceBase.h"
#include "Pooling/ActorPoolInterface.h"
#include "ConnectIt_GridPiece.generated.h"

/*
 * base class for blueprints to ensure we are adding all necessary interfaces
 */
UCLASS(BlueprintType, Blueprintable)
class CONNECTIT_API AConnectIt_GridPiece : public AGridPieceBase,
	public IGridFactionInterface,
	public IGridLevelInterface,
	public IGridTileHandler,
	public IActorPoolInterface
{
	GENERATED_BODY()

public:
	AConnectIt_GridPiece();

	// Set faction and trigger visual update
	// Called immediately after retrieval from pool
	UFUNCTION(BlueprintCallable, Category = "ConnectIt|Piece")
	void InitialisePiece(int32 InFactionID, FGridPosition InGridPosition);

	// Faction this piece belongs to
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_FactionID,
		Category = "ConnectIt|Piece")
	int32 FactionID = -1;

	// Grid position this piece occupies
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "ConnectIt|Piece")
	FGridPosition OccupiedPosition;

	// IActorPoolInterface
	// Blueprint implements the actual visual setup/teardown
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ActivatePoolObject();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void DeactivatePoolObject();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	// Called when FactionID replicates to clients
	// Blueprint override to update mesh, material, particle etc.
	UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Piece")
	void OnFactionVisualUpdate(int32 InFactionID);
	virtual void OnFactionVisualUpdate_Implementation(int32 InFactionID);

private:

	UFUNCTION()
	void OnRep_FactionID();
	
};
