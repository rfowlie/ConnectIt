// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "Board/BoardStateComponentBase.h"
#include "ConnectItBoardStateComponent.generated.h"


UCLASS(ClassGroup=(ConnectIt), meta=(BlueprintSpawnableComponent))
class CONNECTIT_API UConnectItBoardStateComponent : public UBoardStateComponentBase
{
    GENERATED_BODY()

public:

    UConnectItBoardStateComponent();

    // --- Server API ---
    // Only the board manager calls these

    // Initialises the board state from a set of registered tile positions
    // Called once at game start after all tiles register with subsystem
    void InitialiseBoardState(
        const TArray<FGridPosition>& TilePositions,
        int32 NumFactions,
        float InitialMultiplier = 1.0f);

    // Captures current as previous as applies new state
    // Fires OnBoardStateChanged on server immediately
    // Clients receive via OnRep
    void ApplyAndBroadcast(const FConnectItBoardState& NewState);

    // --- Read API ---
    // Interpreters and game logic call these

    FConnectItBoardStateSnapshot GetBoardSnapshot() const;
    
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

    // Convenience -- reads from current state
    UFUNCTION(BlueprintPure, Category = "Board State")
    bool IsTileValidForPlacement(FGridPosition Position) const
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