// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interface/GridBoardActor.h"
#include "GridTileBase.generated.h"

class AGridPieceBase;
class UGridHoverComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridTileBaseBeginCursorOver, AGridTileBase*, GridTileBase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGridTileCursorStateChanged, AGridTileBase*, Tile,
	FGameplayTag, Previous, FGameplayTag, Current);

UCLASS()
class UNREALGRIDMECHANICS_API AGridTileBase : public AActor, public IGridBoardActor
{
	GENERATED_BODY()

public:
	AGridTileBase();

	// Raises hover begin / end and click from the tile's cursor events, in
	// C++ (no Blueprint wiring). Every tile class has one.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Tile")
	TObjectPtr<UGridHoverComponent> HoverComponent;

	// Deprecated: forwarded from HoverComponent's hover begin for one release,
	// so existing Blueprints keep working. Don't broadcast it by hand (the
	// component raises hover now); listen to the component or
	// UGridHoverSubsystem instead.
	UPROPERTY(BlueprintAssignable, BlueprintCallable, meta = (DeprecatedProperty, DeprecationMessage =
		"The hover component raises hover now: remove manual broadcasts; listen to HoverComponent or UGridHoverSubsystem."))
	FGridTileBaseBeginCursorOver OnGridTileBeginCursorOver;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Tile")
	void SendGameplayTag(FGameplayTag StateTag);

	// --- Cursor state ---
	// The look the active player action has put this tile in
	// (Grid.CursorState.*, or any tag the action chooses; no tag = neutral).
	// Actions set it, tiles never choose it. It's its own call, apart from
	// SendGameplayTag, so a tile knows it's being given a cursor state.

	// Stores NewState and raises OnCursorStateChanged; nothing if unchanged
	UFUNCTION(BlueprintCallable, Category = "Grid Tile|Cursor")
	void SetCursorState(FGameplayTag NewState);

	// Back to neutral (no tag)
	UFUNCTION(BlueprintCallable, Category = "Grid Tile|Cursor")
	void ClearCursorState() { SetCursorState(FGameplayTag()); }

	UFUNCTION(BlueprintPure, Category = "Grid Tile|Cursor")
	FGameplayTag GetCursorState() const { return CursorState; }

	// After OnCursorStateChanged, for anything else that shows the tile's state
	UPROPERTY(BlueprintAssignable, Category = "Grid Tile|Cursor")
	FGridTileCursorStateChanged OnTileCursorStateChanged;

	// IGridBoardActor: nothing here -- a game's tile reads its own board data
	virtual void RefreshFromBoard_Implementation() override {}

protected:
	virtual void BeginPlay() override;

	// Show the new cursor state here (Previous -> Current; no tag = neutral)
	UFUNCTION(BlueprintNativeEvent, Category = "Grid Tile|Cursor")
	void OnCursorStateChanged(FGameplayTag Previous, FGameplayTag Current);
	virtual void OnCursorStateChanged_Implementation(FGameplayTag Previous, FGameplayTag Current) {}

private:
	FGameplayTag CursorState;

	UFUNCTION()
	void ForwardHoverBegin(UGridHoverComponent* Component);
	
};
