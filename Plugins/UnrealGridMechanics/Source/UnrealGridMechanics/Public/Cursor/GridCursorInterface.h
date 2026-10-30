// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GridCursorInterface.generated.h"

class AGridTileBase;

UINTERFACE(BlueprintType)
class UNREALGRIDMECHANICS_API UGridCursor : public UInterface
{
	GENERATED_BODY()
};

// A cursor actor a player action shows on tiles (UTurnBasedPlayerAction's
// cursor helpers). Mandatory for every cursor class: an action refuses one
// that doesn't implement it. Cursors come from the actor pool -- the pool's
// activate / deactivate means in use or not; Show / Hide means visible while
// the action holds it.
class UNREALGRIDMECHANICS_API IGridCursor
{
	GENERATED_BODY()

public:
	// Go to Tile; Location is the tile's location plus the action's cursor
	// offset. bReactivate: replay arrival effects (particles...) -- set when
	// the cursor moves to a different tile.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Mechanics|Cursor")
	void MoveCursor(AGridTileBase* Tile, FVector Location, bool bReactivate);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Mechanics|Cursor")
	void ShowCursor();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Mechanics|Cursor")
	void HideCursor();
};
