// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Hover/GridHoverComponent.h"
#include "Subsystem/GridHoverSubsystem.h"
#include "Tile/GridTileBase.h"
#include "Piece/GridPieceBase.h"
#include "Components/BoxComponent.h"
#include "GridHoverTestTypes.generated.h"


// --- Test-only types (UnrealGridMechanics.Hover). Not meant for real use. ---

// Counts what a hover component broadcasts
UCLASS(Hidden, NotBlueprintable)
class UGridHoverTestListener : public UObject
{
    GENERATED_BODY()

public:

    int32 Begins = 0;
    int32 Ends = 0;
    int32 Clicks = 0;

    void Listen(UGridHoverComponent* Component)
    {
        Component->OnHoverBegin.AddDynamic(this, &UGridHoverTestListener::HandleBegin);
        Component->OnHoverEnd.AddDynamic(this, &UGridHoverTestListener::HandleEnd);
        Component->OnClicked.AddDynamic(this, &UGridHoverTestListener::HandleClicked);
    }

    UFUNCTION()
    void HandleBegin(UGridHoverComponent* Component) { Begins++; }

    UFUNCTION()
    void HandleEnd(UGridHoverComponent* Component) { Ends++; }

    UFUNCTION()
    void HandleClicked(UGridHoverComponent* Component) { Clicks++; }
};

// Counts what UGridHoverSubsystem relays for tiles
UCLASS(Hidden, NotBlueprintable)
class UGridHoverTestTileListener : public UObject
{
    GENERATED_BODY()

public:

    int32 Begins = 0;
    int32 Ends = 0;
    int32 Clicks = 0;
    TObjectPtr<AGridTileBase> LastTile = nullptr;

    void Listen(UGridHoverSubsystem* Subsystem)
    {
        Subsystem->OnGridTileHoverChanged.AddDynamic(this, &UGridHoverTestTileListener::HandleBegin);
        Subsystem->OnGridTileHoverEnded.AddDynamic(this, &UGridHoverTestTileListener::HandleEnd);
        Subsystem->OnGridTileClicked.AddDynamic(this, &UGridHoverTestTileListener::HandleClicked);
    }

    UFUNCTION()
    void HandleBegin(AGridTileBase* Tile) { Begins++; LastTile = Tile; }

    UFUNCTION()
    void HandleEnd(AGridTileBase* Tile) { Ends++; LastTile = Tile; }

    UFUNCTION()
    void HandleClicked(AGridTileBase* Tile) { Clicks++; LastTile = Tile; }
};

// Records what a tile's cursor state changes broadcast
UCLASS(Hidden, NotBlueprintable)
class UGridCursorStateTestListener : public UObject
{
    GENERATED_BODY()

public:

    int32 Changes = 0;
    FGameplayTag LastPrevious;
    FGameplayTag LastCurrent;

    void Listen(AGridTileBase* Tile)
    {
        Tile->OnTileCursorStateChanged.AddDynamic(this, &UGridCursorStateTestListener::HandleChanged);
    }

    UFUNCTION()
    void HandleChanged(AGridTileBase* Tile, FGameplayTag Previous, FGameplayTag Current)
    {
        Changes++;
        LastPrevious = Previous;
        LastCurrent = Current;
    }
};

// A tile and a piece with a box as their root primitive (what a Blueprint's
// mesh would be): the cursor channel response comes from the C++ base
UCLASS(Hidden, NotBlueprintable, NotPlaceable)
class AGridTestBoxTile : public AGridTileBase
{
    GENERATED_BODY()

public:

    AGridTestBoxTile() { RootComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Box")); }
};

UCLASS(Hidden, NotBlueprintable, NotPlaceable)
class AGridTestBoxPiece : public AGridPieceBase
{
    GENERATED_BODY()

public:

    AGridTestBoxPiece() { RootComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Box")); }
};

// A hover component whose CanHover is bAllow
UCLASS(Hidden, NotBlueprintable)
class UGridHoverTestVetoComponent : public UGridHoverComponent
{
    GENERATED_BODY()

public:

    bool bAllow = false;

    virtual bool CanHover_Implementation() const override { return bAllow; }
};
