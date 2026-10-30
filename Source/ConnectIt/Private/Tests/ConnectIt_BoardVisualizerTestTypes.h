// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/Actor/ConnectIt_PieceBase.h"
#include "Board/Actor/ConnectIt_TileBase.h"
#include "Board/ConnectIt_EventVisual.h"
#include "Board/ConnectIt_VisualHandle.h"
#include "Framework/Library/ConnectIt_BoardEventLibrary.h"
#include "Components/SceneComponent.h"
#include "ConnectIt_BoardVisualizerTestTypes.generated.h"


// --- Test-only types (ConnectIt.Visuals.*). Not meant for real use. ---

// A tile that records what its hooks are given
UCLASS(Hidden, NotBlueprintable, NotPlaceable)
class AConnectIt_TestTile : public AConnectIt_TileBase
{
    GENERATED_BODY()

public:

    // A root, so it stands where it is spawned
    AConnectIt_TestTile() { RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root")); }

    TArray<FGameplayTag> EnterTags;
    TArray<int32> EnterFactions;
    TArray<FGameplayTag> ExitTags;
    TArray<FConnectItBoardEvent_MultiplierChanged> Multipliers;

    // On Lock Changed: hold the handle, wander off the cell, and wait for
    // ReleaseHeld (a visual mid-animation)
    bool bHoldOnLock = false;
    TObjectPtr<UConnectIt_VisualHandle> Held = nullptr;

    void ReleaseHeld()
    {
        if (Held) Held->Release(this);
        Held = nullptr;
    }

protected:

    virtual void OnPieceEnter_Implementation(const FInstancedStruct& Event, int32 Faction, UConnectIt_VisualHandle* Handle) override
    {
        EnterTags.Add(UConnectIt_BoardEventLibrary::GetBoardEventTag(Event));
        EnterFactions.Add(Faction);
    }

    virtual void OnPieceExit_Implementation(const FInstancedStruct& Event, int32 Faction, UConnectIt_VisualHandle* Handle) override
    {
        ExitTags.Add(UConnectIt_BoardEventLibrary::GetBoardEventTag(Event));
    }

    virtual void OnMultiplierChanged_Implementation(const FConnectItBoardEvent_MultiplierChanged& Event, UConnectIt_VisualHandle* Handle) override
    {
        Multipliers.Add(Event);
    }

    virtual void OnLockChanged_Implementation(const FConnectItBoardEvent_TileLockChanged& Event, UConnectIt_VisualHandle* Handle) override
    {
        if (!bHoldOnLock) return;
        Handle->Hold(this);
        Held = Handle;
        AddActorWorldOffset(FVector(0.f, 30.f, 0.f));
    }
};

// A piece that records what its hooks are given
UCLASS(Hidden, NotBlueprintable, NotPlaceable)
class AConnectIt_TestPiece : public AConnectIt_PieceBase
{
    GENERATED_BODY()

public:

    AConnectIt_TestPiece() { RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root")); }

    TArray<FConnectItBoardEvent_PiecePlaced> Placed;
    TArray<FConnectItBoardEvent_PieceMoved> Moved;
    TArray<FConnectItBoardEvent_PiecesSwapped> Swapped;
    TArray<FConnectItBoardEvent_PieceCaptured> Captured;
    int32 Cleared = 0;

    // On Captured: wander off the tile and forget to come back (a broken visual)
    bool bWanderOnCapture = false;

protected:

    virtual void OnPlaced_Implementation(const FConnectItBoardEvent_PiecePlaced& Event, UConnectIt_VisualHandle* Handle) override { Placed.Add(Event); }
    virtual void OnMoved_Implementation(const FConnectItBoardEvent_PieceMoved& Event, UConnectIt_VisualHandle* Handle) override { Moved.Add(Event); }
    virtual void OnSwapped_Implementation(const FConnectItBoardEvent_PiecesSwapped& Event, UConnectIt_VisualHandle* Handle) override { Swapped.Add(Event); }
    virtual void OnClearedByScore_Implementation(const FConnectItBoardEvent_Scored& Event, UConnectIt_VisualHandle* Handle) override { Cleared++; }

    virtual void OnCaptured_Implementation(const FConnectItBoardEvent_PieceCaptured& Event, UConnectIt_VisualHandle* Handle) override
    {
        Captured.Add(Event);
        if (bWanderOnCapture) AddActorWorldOffset(FVector(100.f, 0.f, 0.f));
    }
};

// A Piece Placed event visual that records what it plays (replacing the default)
UCLASS(Hidden, NotBlueprintable, NotPlaceable)
class AConnectIt_TestPlacedVisual : public AConnectIt_EventVisual_PiecePlaced
{
    GENERATED_BODY()

public:

    int32 Plays = 0;
    FConnectItBoardEvent_PiecePlaced LastEvent;

protected:

    // Also run the default (the actors' hooks)
    virtual bool CallsDefault() const { return false; }

    virtual void OnPlay_Implementation(const FConnectItBoardEvent_PiecePlaced& Event, UConnectIt_VisualHandle* Handle) override
    {
        Plays++;
        LastEvent = Event;
        if (CallsDefault()) PlayDefault();
    }
};

// The same, also calling Play Default
UCLASS(Hidden, NotBlueprintable, NotPlaceable)
class AConnectIt_TestPlacedVisualWithDefault : public AConnectIt_TestPlacedVisual
{
    GENERATED_BODY()

protected:

    virtual bool CallsDefault() const override { return true; }
};
