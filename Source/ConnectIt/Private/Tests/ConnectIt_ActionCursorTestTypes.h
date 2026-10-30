// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action/TurnBasedPlayerAction.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridCursorStateTags.h"
#include "Cursor/GridCursorInterface.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "ConnectIt_ActionCursorTestTypes.generated.h"


// --- Test-only types (ConnectIt.PlayerAction.Cursors). Not meant for real use. ---

// A cursor that records what it's told
UCLASS(Hidden, NotBlueprintable)
class AConnectIt_TestCursor : public AActor, public IGridCursor
{
    GENERATED_BODY()

public:

    AConnectIt_TestCursor() { RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root")); }

    bool bShown = false;
    int32 Moves = 0;
    int32 Reactivations = 0;
    TObjectPtr<AGridTileBase> LastTile = nullptr;

    virtual void MoveCursor_Implementation(AGridTileBase* Tile, FVector Location, bool bReactivate) override
    {
        SetActorLocation(Location);
        LastTile = Tile;
        Moves++;
        if (bReactivate) Reactivations++;
    }

    virtual void ShowCursor_Implementation() override { bShown = true; }
    virtual void HideCursor_Implementation() override { bShown = false; }
};

// A second cursor class (for the per-tile class swap)
UCLASS(Hidden, NotBlueprintable)
class AConnectIt_TestCursorOther : public AConnectIt_TestCursor
{
    GENERATED_BODY()
};

// Every tile is a valid hover. The hover cursor is HoverClass, or OtherClass on
// OtherTile. On hover it sets the tile Hovered and never clears it, so only the
// action's own cleanup can bring the tile back to neutral.
UCLASS(Hidden, NotBlueprintable)
class UConnectIt_TestCursorAction : public UTurnBasedPlayerAction
{
    GENERATED_BODY()

public:

    UConnectIt_TestCursorAction() { bRequiresSelection = true; }

    TSubclassOf<AActor> HoverClass;
    TSubclassOf<AActor> OtherClass;
    TObjectPtr<AGridTileBase> OtherTile = nullptr;

protected:

    virtual void HandleValidHover_Implementation(AGridTileBase* Tile) override
    {
        SetTileCursorState(Tile, Grid_CursorState_Hovered);
        ShowHoverCursor(Tile == OtherTile ? OtherClass : HoverClass, Tile);
    }

    virtual void HandleHoverCleared_Implementation(AGridTileBase* PreviousTile) override
    {
        HideHoverCursor();
    }
};
