// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interface/GridFactionInterface.h"
#include "Interface/GridLevelInterface.h"
#include "Interface/GridTileHandler.h"
#include "Piece/GridPieceBase.h"
#include "Pooling/ActorPoolInterface.h"
#include "ConnectIt_PieceBase.generated.h"

class UConnectIt_VisualHandle;

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

	// The board visualizer's: the piece now stands at Position
	void SetBoardPosition(const FGridPosition& Position);

	// --- Piece hooks ---
	// The board visualizer (AConnectIt_BoardVisualizer) calls these as board
	// events play, after the registry has followed the event. Each is handed
	// the event's handle: Hold it while animating, Release when done (the
	// event waits). Defaults do nothing. The piece ends on its tile whatever
	// the hook does (the visualizer places it once the event's visuals finish).

	// The piece arrived (placed, a starting piece, a tile added with it)
	UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Piece Hooks")
	void OnPlaced(UConnectIt_VisualHandle* Handle);

	// The piece moved From -> To (a power's move, riding a shifted tile)
	UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Piece Hooks")
	void OnMoved(FGridPosition From, FGridPosition To, UConnectIt_VisualHandle* Handle);

	// The piece traded places with another: From -> To
	UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Piece Hooks")
	void OnSwapped(FGridPosition From, FGridPosition To, UConnectIt_VisualHandle* Handle);

	// The piece changed owner where it stands
	UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Piece Hooks")
	void OnCaptured(int32 NewFaction, UConnectIt_VisualHandle* Handle);

	// The piece was taken off the board (removed, or went with its tile); it
	// returns to the pool once the event's visuals are done
	UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Piece Hooks")
	void OnRemoved(UConnectIt_VisualHandle* Handle);

	// A score cleared the piece; it returns to the pool once the event's
	// visuals are done
	UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Piece Hooks")
	void OnClearedByScore(UConnectIt_VisualHandle* Handle);

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

	virtual void OnPlaced_Implementation(UConnectIt_VisualHandle* Handle) {}
	virtual void OnMoved_Implementation(FGridPosition From, FGridPosition To, UConnectIt_VisualHandle* Handle) {}
	virtual void OnSwapped_Implementation(FGridPosition From, FGridPosition To, UConnectIt_VisualHandle* Handle) {}
	virtual void OnCaptured_Implementation(int32 NewFaction, UConnectIt_VisualHandle* Handle) {}
	virtual void OnRemoved_Implementation(UConnectIt_VisualHandle* Handle) {}
	virtual void OnClearedByScore_Implementation(UConnectIt_VisualHandle* Handle) {}

	// Lets go of any hold it still has on a playing event
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

};
