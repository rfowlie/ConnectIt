// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "GameFramework/Actor.h"
#include "Pooling/ActorPoolInterface.h"
#include "GridPieceBase.generated.h"

class AGridPieceBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPieceVisualComplete, AGridPieceBase*, Piece);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridPieceBaseBeginCursorOver, AGridPieceBase*, GridPieceBase);

/*
 * An actor that is considered a piece but does not use GAS
 */
UCLASS(BlueprintType, Blueprintable)
class UNREALGRIDMECHANICS_API AGridPieceBase : public AActor, public IActorPoolInterface
{
	GENERATED_BODY()

public:
	AGridPieceBase();

	UFUNCTION(BlueprintPure, Category = "Grid")
	FGridPosition GetGridPosition() const { return GridPosition; }

	// Allow piece Blueprints to customize what counts as a cursor-over
	// (mirror of AGridTileBase::OnGridTileBeginCursorOver). Broadcast from
	// the piece Blueprint's native "Begin Cursor Over" event.
	// UGridHoverSubsystem chains this into OnGridPieceHoverChanged for every
	// registered (i.e. currently active, pool-checked-out) piece.
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Grid")
	FGridPieceBaseBeginCursorOver OnGridPieceBeginCursorOver;

	// Fired once this piece's own activation (spawn-in) visual effect has
	// genuinely finished, not merely started -- a Blueprint subclass with
	// no meaningful effect should call NotifyActivationVisualComplete
	// immediately from its own ActivatePoolObject. Listeners (e.g. a
	// piece-spawning system gating its own completion on this) bind here
	// rather than assuming activation is instantaneous.
	UPROPERTY(BlueprintAssignable, Category = "Grid|Pooling")
	FOnPieceVisualComplete OnActivationVisualComplete;

	// Mirror of OnActivationVisualComplete for deactivation (despawn-out).
	UPROPERTY(BlueprintAssignable, Category = "Grid|Pooling")
	FOnPieceVisualComplete OnDeactivationVisualComplete;

	// Called by Blueprint (or C++) once this piece's activation visual is
	// actually done -- broadcasts OnActivationVisualComplete(this).
	UFUNCTION(BlueprintCallable, Category = "Grid|Pooling")
	void NotifyActivationVisualComplete() { OnActivationVisualComplete.Broadcast(this); }

	// Mirror of NotifyActivationVisualComplete for deactivation.
	UFUNCTION(BlueprintCallable, Category = "Grid|Pooling")
	void NotifyDeactivationVisualComplete() { OnDeactivationVisualComplete.Broadcast(this); }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	FGridPosition GridPosition;
};
