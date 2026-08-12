// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "Board/BoardStateComponentBase.h"
#include "ConnectIt_BoardStateComponent.generated.h"


UCLASS(ClassGroup=(ConnectIt), meta=(BlueprintSpawnableComponent))
class CONNECTIT_API UConnectIt_BoardStateComponent : public UBoardStateComponentBase
{
    GENERATED_BODY()

public:

    UConnectIt_BoardStateComponent();

    // --- Server API ---
    // Only the board manager calls these

    // Initialises the board state from a set of registered tile positions
    // Called once at game start after all tiles register with subsystem
    void InitialiseBoardState(
        const TArray<FGridPosition>& TilePositions,
        int32 NumFactions,
        float InitialMultiplier = 1.0f);

    // Captures current as previous, applies new state, and stores what
    // specifically changed (ChangeEvent) so it replicates atomically
    // alongside the state it describes.
    // Fires OnBoardStateChanged on server immediately
    // Clients receive via OnRep -- AConnectIt_BoardManager reads ChangeEvent
    // from that same signal on both machines to drive gated visual
    // sequencing (see HandleBoardStateChanged)
    void ApplyAndBroadcast(
        const FConnectItBoardState& NewState,
        const FConnectItBoardChangeEvent& ChangeEvent);

    // --- Read API ---
    // Interpreters and game logic call these

    const FConnectItBoardStateSnapshot* GetBoardSnapshot() const;

    // void GetBoardSnapShotCurrent(const FConnectItBoardState* OutSnapshot);
    const FConnectItBoardState* GetBoardSnapShotCurrent() const;
    
    const FConnectItBoardState* GetBoardSnapShotPrevious() const;
    
    UFUNCTION(BlueprintPure, Category = "Board State")
    const FConnectItBoardState& GetCurrentState() const
    {
        return BoardSnapshot.CurrentState;
    }

    UFUNCTION(BlueprintPure, Category = "Board State")
    const FConnectItBoardState& GetPreviousState() const
    {
        return BoardSnapshot.PreviousState;
    }

    // What specifically changed on the most recent ApplyAndBroadcast call.
    // Listeners bound to a UGameEventTaskSubsystem board event tag (which
    // carries no parameters) call this to read payload data -- position,
    // faction, points scored, winner -- instead of it being threaded
    // through a delegate parameter
    UFUNCTION(BlueprintPure, Category = "Board State")
    const FConnectItBoardChangeEvent& GetChangeEvent() const
    {
        return BoardSnapshot.ChangeEvent;
    }

    // Convenience -- reads from current state
    UFUNCTION(BlueprintPure, Category = "Board State")
    bool IsTileValidForPlacement(const FGridPosition Position) const
    {
        return BoardSnapshot.CurrentState.IsTileValidForPlacement(Position);
    }

    UFUNCTION(BlueprintPure, Category = "Board State")
    float GetFactionScore(int32 FactionSlot) const
    {
        return BoardSnapshot.CurrentState.GetScore(FactionSlot);
    }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

    // THE ONE REPLICATED PROPERTY
    // Previous and current state replicate together atomically
    // OnRep fires void OnBoardStateChanged delegate on clients
    UPROPERTY(ReplicatedUsing = OnRep_BoardSnapshot)
    FConnectItBoardStateSnapshot BoardSnapshot;

    // Captures current state as previous before applying new state
    // Server only -- called inside ApplyAndBroadcast
    void CaptureSnapshot();

    UFUNCTION()
    void OnRep_BoardSnapshot();
};