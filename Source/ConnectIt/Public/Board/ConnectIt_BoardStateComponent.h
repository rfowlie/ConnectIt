// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "Board/BoardStateComponentBase.h"
#include "ConnectIt_BoardStateComponent.generated.h"

// Everything a debug widget needs to know about this component's current
// values in one call -- used to seed initial state once, right after
// binding, through the same events used for later reactive updates (see
// UDWidgetBase's own class comment for the convention this follows).
// OnBoardStateChanged itself stays zero-param (shared with every board
// interpreter, not just debug widgets) -- this just wraps the same
// GetCurrentState()/GetChangeEvent() reads a listener already does after
// that ping.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItBoardStateInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FConnectItBoardState CurrentState;

    UPROPERTY(BlueprintReadOnly)
    FConnectItBoardChangeEvent LastChangeEvent;
};

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
    void SetBoardState(
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

    // Everything a debug widget needs, in one call -- see
    // FConnectItBoardStateInfo's own comment.
    UFUNCTION(BlueprintPure, Category = "Board State")
    FConnectItBoardStateInfo GetInfo() const
    {
        return { BoardSnapshot.CurrentState, BoardSnapshot.ChangeEvent };
    }

    /*
     *  TODO: remove, let's try not repeat ourselves
     *  unless we opt to move all the helper functions out of the struct into here
     */
    // // Convenience -- reads from current state
    // UFUNCTION(BlueprintPure, Category = "Board State")
    // bool IsTileValidForPlacement(const FGridPosition Position) const
    // {
    //     return BoardSnapshot.CurrentState.IsTileValidForPlacement(Position);
    // }
    //
    // UFUNCTION(BlueprintPure, Category = "Board State")
    // float GetFactionScore(int32 FactionSlot) const
    // {
    //     return BoardSnapshot.CurrentState.GetScore(FactionSlot);
    // }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

    // THE ONE REPLICATED PROPERTY
    // Previous and current state replicate together atomically
    // OnRep fires void OnBoardStateChanged delegate on clients
    UPROPERTY(ReplicatedUsing = OnRep_BoardSnapshot)
    FConnectItBoardStateSnapshot BoardSnapshot;

    UFUNCTION()
    void OnRep_BoardSnapshot();

    // Reads the just-recorded ChangeEvent and enqueues this board change's
    // event tags on UGameEventTaskSubsystem, one QueueTagContainer call per
    // event, in the fixed order shift/piece-placed, then line-scored, then
    // player-win (shift and piece-placed are always mutually exclusive on a
    // single ChangeEvent). Called symmetrically from both SetBoardState
    // (server) and OnRep_BoardSnapshot (client), right after BroadcastChange
    // -- ported from the now-deprecated UConnectIt_BoardSequencerComponent,
    // which used to derive the same step list from a separate listener
    // rather than the component that already owns this data.
    void EnqueueBoardEventTags() const;
};