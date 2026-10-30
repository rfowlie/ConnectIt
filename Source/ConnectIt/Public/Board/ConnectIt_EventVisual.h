// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "ConnectIt_EventVisual.generated.h"

class AConnectIt_BoardVisualizer;
class UConnectIt_VisualHandle;


// An artist's visuals for one kind of board event, played instead of the
// board visualizer's own function for it (On Piece Placed...). Make a
// Blueprint of that event kind's class (ConnectIt_EventVisual_PiecePlaced...),
// implement On Play -- it gets a copy of the event and its handle (Hold while
// animating, Release when done) -- and list it in the level config's Event
// Visuals. Call Play Default (from On Play) to also run the visualizer's own
// function, i.e. the actors' hooks. Registry bookkeeping and the reconcile
// step run either way. Spawned the first time its event plays; goes with
// the map. Each event kind's class declares its event tag.
UCLASS(Abstract, Blueprintable, NotPlaceable)
class CONNECTIT_API AConnectIt_EventVisual : public AActor
{
    GENERATED_BODY()

public:

    AConnectIt_EventVisual();

    // The board event this visual plays
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Event Visual")
    FGameplayTag GetEventTag() const { return GetDeclaredEventTag(); }

    // The event tag a visual class plays (none for the base or a null class)
    static FGameplayTag GetEventTagOf(TSubclassOf<AConnectIt_EventVisual> VisualClass);

    // Runs the visualizer's own function for the playing event -- what would
    // play without this visual. Call it from On Play, before releasing.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Event Visual")
    void PlayDefault();

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Event Visual")
    AConnectIt_BoardVisualizer* GetVisualizer() const { return Visualizer; }

    // The visualizer's: plays Payload (this visual's event) with Handle
    void PlayEvent(AConnectIt_BoardVisualizer* InVisualizer, const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle);

protected:

    // Each event kind's class: its tag, and its typed On Play
    virtual FGameplayTag GetDeclaredEventTag() const { return FGameplayTag(); }
    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) {}

private:

    UPROPERTY()
    TObjectPtr<AConnectIt_BoardVisualizer> Visualizer = nullptr;

    UPROPERTY()
    TObjectPtr<UConnectIt_VisualHandle> ActiveHandle = nullptr;

    // The playing event (for Play Default)
    FInstancedStruct ActivePayload;
};


// --- One class per kind of board event (thin; a new event kind adds one) ---

// Visuals for Board Seeded
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_BoardSeeded : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_BoardSeeded& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_BoardSeeded& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_BoardSeeded().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_BoardSeeded* Event = Payload.GetPtr<FConnectItBoardEvent_BoardSeeded>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Piece Placed
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_PiecePlaced : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_PiecePlaced& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_PiecePlaced& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_PiecePlaced().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_PiecePlaced* Event = Payload.GetPtr<FConnectItBoardEvent_PiecePlaced>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Pieces Swapped
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_PiecesSwapped : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_PiecesSwapped& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_PiecesSwapped& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_PiecesSwapped().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_PiecesSwapped* Event = Payload.GetPtr<FConnectItBoardEvent_PiecesSwapped>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Board Shifted
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_BoardShifted : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_BoardShifted& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_BoardShifted& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_BoardShifted().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_BoardShifted* Event = Payload.GetPtr<FConnectItBoardEvent_BoardShifted>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Piece Captured
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_PieceCaptured : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_PieceCaptured& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_PieceCaptured& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_PieceCaptured().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_PieceCaptured* Event = Payload.GetPtr<FConnectItBoardEvent_PieceCaptured>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Piece Removed
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_PieceRemoved : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_PieceRemoved& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_PieceRemoved& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_PieceRemoved().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_PieceRemoved* Event = Payload.GetPtr<FConnectItBoardEvent_PieceRemoved>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Tile Multiplier Destroyed
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_TileMultiplierDestroyed : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_TileMultiplierDestroyed& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_TileMultiplierDestroyed& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_TileMultiplierDestroyed().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_TileMultiplierDestroyed* Event = Payload.GetPtr<FConnectItBoardEvent_TileMultiplierDestroyed>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Tile Active Toggled
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_TileActiveToggled : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_TileActiveToggled& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_TileActiveToggled& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_TileActiveToggled().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_TileActiveToggled* Event = Payload.GetPtr<FConnectItBoardEvent_TileActiveToggled>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Goals Changed
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_GoalsChanged : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_GoalsChanged& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_GoalsChanged& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_GoalsChanged().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_GoalsChanged* Event = Payload.GetPtr<FConnectItBoardEvent_GoalsChanged>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Tiles Removed
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_TilesRemoved : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_TilesRemoved& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_TilesRemoved& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_TilesRemoved().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_TilesRemoved* Event = Payload.GetPtr<FConnectItBoardEvent_TilesRemoved>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Tiles Added
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_TilesAdded : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_TilesAdded& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_TilesAdded& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_TilesAdded().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_TilesAdded* Event = Payload.GetPtr<FConnectItBoardEvent_TilesAdded>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Phase Changed
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_PhaseChanged : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_PhaseChanged& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_PhaseChanged& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_PhaseChanged().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_PhaseChanged* Event = Payload.GetPtr<FConnectItBoardEvent_PhaseChanged>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Layout Changed
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_LayoutChanged : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_LayoutChanged& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_LayoutChanged& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_LayoutChanged().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_LayoutChanged* Event = Payload.GetPtr<FConnectItBoardEvent_LayoutChanged>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Piece Moved
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_PieceMoved : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_PieceMoved& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_PieceMoved& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_PieceMoved().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_PieceMoved* Event = Payload.GetPtr<FConnectItBoardEvent_PieceMoved>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Multiplier Changed
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_MultiplierChanged : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_MultiplierChanged& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_MultiplierChanged& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_MultiplierChanged().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_MultiplierChanged* Event = Payload.GetPtr<FConnectItBoardEvent_MultiplierChanged>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Tile Lock Changed
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_TileLockChanged : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_TileLockChanged& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_TileLockChanged& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_TileLockChanged().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_TileLockChanged* Event = Payload.GetPtr<FConnectItBoardEvent_TileLockChanged>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Chain Partner Changed
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_ChainPartnerChanged : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_ChainPartnerChanged& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_ChainPartnerChanged& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_ChainPartnerChanged().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_ChainPartnerChanged* Event = Payload.GetPtr<FConnectItBoardEvent_ChainPartnerChanged>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Scored
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_Scored : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_Scored& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_Scored& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_Scored().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_Scored* Event = Payload.GetPtr<FConnectItBoardEvent_Scored>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Tile Power Triggered
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_TilePowerTriggered : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_TilePowerTriggered& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_TilePowerTriggered& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_TilePowerTriggered().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_TilePowerTriggered* Event = Payload.GetPtr<FConnectItBoardEvent_TilePowerTriggered>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Win Checkpoint Reached
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_WinCheckpointReached : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_WinCheckpointReached& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_WinCheckpointReached& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_WinCheckpointReached().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_WinCheckpointReached* Event = Payload.GetPtr<FConnectItBoardEvent_WinCheckpointReached>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Lose Condition Met
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_LoseConditionMet : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_LoseConditionMet& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_LoseConditionMet& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_LoseConditionMet().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_LoseConditionMet* Event = Payload.GetPtr<FConnectItBoardEvent_LoseConditionMet>()) OnPlay(*Event, Handle);
    }
};

// Visuals for Game Won
UCLASS(Abstract, Blueprintable)
class CONNECTIT_API AConnectIt_EventVisual_GameWon : public AConnectIt_EventVisual
{
    GENERATED_BODY()

protected:

    // The event plays (see AConnectIt_EventVisual). Default: Play Default.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Event Visual")
    void OnPlay(const FConnectItBoardEvent_GameWon& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPlay_Implementation(const FConnectItBoardEvent_GameWon& Event, UConnectIt_VisualHandle* Handle) { PlayDefault(); }

    virtual FGameplayTag GetDeclaredEventTag() const override { return FConnectItBoardEvent_GameWon().GetEventTag(); }

    virtual void Play(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle) override
    {
        if (const FConnectItBoardEvent_GameWon* Event = Payload.GetPtr<FConnectItBoardEvent_GameWon>()) OnPlay(*Event, Handle);
    }
};
