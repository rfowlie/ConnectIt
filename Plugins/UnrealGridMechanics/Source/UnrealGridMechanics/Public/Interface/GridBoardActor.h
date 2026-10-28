// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GridBoardActor.generated.h"

UINTERFACE(BlueprintType)
class UNREALGRIDMECHANICS_API UGridBoardActor : public UInterface
{
	GENERATED_BODY()
};

/**
 * A grid actor (tile, piece) that can update itself from the board. Visuals
 * call RefreshFromBoard after a change reaches the board they show; the actor
 * finds its own position and board data, so the caller doesn't need to know
 * what kind of tile or piece it is. Each game implements it on its actors
 * (the plugin can't know a game's board): AGridTileBase and AGridPieceBase
 * implement it as a no-op. Not for timed visual effects -- those stay on the
 * game's event hooks.
 */
class UNREALGRIDMECHANICS_API IGridBoardActor
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid|Board")
	void RefreshFromBoard();
};
