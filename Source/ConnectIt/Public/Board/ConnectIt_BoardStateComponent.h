// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "Board/BoardStateComponentBase.h"
#include "ConnectIt_BoardStateComponent.generated.h"

class UGridPieceRegistryBase;
class UGridTileRegistryBase;
class AGridTileBase;
class UConnectIt_TileCatalogDataAsset;
struct FConnectItRuleSet;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConnectItDisplayBoardChanged);


// Everything a debug widget needs to know about this component's current
// values in one call -- used to seed initial state once, right after
// binding, through the same events used for later reactive updates (see
// UDWidgetBase's own class comment for the convention this follows).
// OnBoardStateChanged itself stays zero-param (shared with every bound
// listener, not just debug widgets) -- this just wraps the same
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
    // Called once at game start after all tiles register with subsystem.
    // InTargetScore stamps FConnectItBoardState::TargetScore up front (see
    // its own comment) -- CheckWinCondition only runs after a placement, so
    // without this the pre-first-move board would read TargetScore == 0.
    // Rules (the match's): the seeded board is settled once under them -- its
    // Sync tile powers run (e.g. a Chained tile locks while its partner is
    // empty) -- so it starts in the state they keep. The settle's events
    // follow Board Seeded, played from the board as built.
    void InitialiseBoardState(
        UGridTileRegistryBase* TileRegistry,
        UGridPieceRegistryBase* PieceRegistry,
        int32 NumFactions,
        float InitialMultiplier = 1.0f,
        float InTargetScore = 0.f,
        const FConnectItRuleSet* Rules = nullptr);

    // The board tile a tile actor starts as (its id is the board's to give):
    // the designer's values -- power tag included -- if it implements
    // IConnectIt_TileDataProvider (a builder tile does), else the defaults
    // with InitialMultiplier. A power tag Catalog (if given) doesn't define
    // is dropped, with a warning. Logs what doesn't fit.
    static FConnectItTileData MakeTileData(const AGridTileBase* Tile, int32 NumFactions,
        float InitialMultiplier = 1.0f, const UConnectIt_TileCatalogDataAsset* Catalog = nullptr);

    // Captures current as previous, applies new state, and stores what
    // specifically changed (ChangeEvent) so it replicates atomically
    // alongside the state it describes.
    // Fires OnBoardStateChanged on server immediately
    // Clients receive via OnRep -- EnqueueBoardEventTags reads ChangeEvent
    // from that same signal on both machines to drive gated visual
    // sequencing
    // Then (server) OnBoardChangeCommitted, and the
    // ConnectIt_Event_BoardChangeComplete marker queued behind the change's
    // events: it completes once they have all played out.
    void SetBoardState(
        const FConnectItBoardState& NewState,
        const FConnectItBoardChangeEvent& ChangeEvent);

    // Server only: a board change has been committed and its events queued
    // (after OnBoardStateChanged), just before its completion marker is
    // queued. Not fired for the initial seed.
    FSimpleMulticastDelegate OnBoardChangeCommitted;

    // --- Read API ---
    // Bound listeners and game logic call these

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

    // Everything that happened in the most recent board change, as an
    // ordered list of events. For a listener reacting to ONE event (bound to
    // a board event tag), use UConnectIt_BoardEventLibrary instead -- by the time an
    // event plays, a newer board change may already have replaced this list.
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

    // --- Display board ---
    // The board as the player has seen it so far, for visuals: it steps along
    // one board event at a time, each applied (FConnectItBoardEvent::ApplyTo,
    // from the event's own payload) when that event's queue entry completes.
    // So a handler playing an event sees the board before it, plus the event.
    // On every machine. Starts as the seeded board (a client: its first
    // snapshot's Previous). It never resets in the middle of a list -- a newer
    // snapshot's events just queue behind. When the last queued event
    // completes it copies Current (the match-level fields no event carries:
    // win progress, target score, game over, turn), snapping to it with a
    // warning if the tiles or scores differ (a snapshot was missed).
    // Game logic (directors, Mediator, AI) reads Current, never this.

    UFUNCTION(BlueprintPure, Category = "Board State")
    const FConnectItBoardState& GetDisplayBoard() const
    {
        return DisplayBoard;
    }

    // Fires each time the display board moves on (an event applied, or the
    // copy of Current once the queue drains)
    UPROPERTY(BlueprintAssignable, Category = "Board State")
    FOnConnectItDisplayBoardChanged OnDisplayBoardChanged;

    // --- Helpers ---

    // get count of pieces a player has, probably a better way to track this
    UFUNCTION(BlueprintPure, Category = "Board State")
    int32 GetFactionPieceCount(const int32 FactionId) const;

    // get all grid positions controlled by a faction
    UFUNCTION(BlueprintPure, Category = "Board State")
    TArray<FGridPosition> GetFactionPiecePositions(const int32 FactionId) const;
    
    UFUNCTION(BlueprintPure, Category = "Board State")
    int32 GetPositionMultiplier(const FGridPosition GridPosition) const;

    UFUNCTION(BlueprintPure, Category = "Board State")
    int32 GetTileMultiplier(const AGridTileBase* GridTile) const;
    
    // Resolved: per-tile/per-state helper queries (IsTileOccupied, GetScore,
    // etc.) live on UConnectIt_BoardStateLibrary, not here -- a
    // UBlueprintFunctionLibrary taking FConnectItBoardState by const&, so it
    // also works on GetPreviousState() and detached/hypothetical states
    // (MinMax), not just this component's own live state. See
    // Docs/UIValueCatalogue.md Gap 4 for the reasoning.

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

    // THE ONE REPLICATED PROPERTY
    // Previous and current state replicate together atomically
    // OnRep fires void OnBoardStateChanged delegate on clients
    UPROPERTY(ReplicatedUsing = OnRep_BoardSnapshot)
    FConnectItBoardStateSnapshot BoardSnapshot;

    UFUNCTION()
    void OnRep_BoardSnapshot();

    // Queues every event of the just-recorded ChangeEvent on
    // UGameEventTaskSubsystem, in list order: one queue entry per event,
    // carrying the event's tag and the event itself as payload. The queue
    // plays them one at a time, waiting for each one's visuals. Called
    // symmetrically from both SetBoardState (server) and OnRep_BoardSnapshot
    // (client), right after BroadcastChange. Counts them for the display
    // board.
    void EnqueueBoardEventTags();

    // See GetDisplayBoard
    FConnectItBoardState DisplayBoard;

    // Whether DisplayBoard has been started (seed, or a client's first snapshot)
    bool bDisplayBoardStarted = false;

    // Board events queued whose entries haven't completed yet
    int32 PendingDisplayEvents = 0;

    // Bound to UGameEventTaskSubsystem::OnAnyTagComplete (before the first
    // event is queued): applies a completing board event to DisplayBoard
    UFUNCTION()
    void HandleQueueEntryComplete(FGameplayTag Tag);

    // DisplayBoard = Current, warning when the tiles or scores differ
    void CatchDisplayBoardUp();
};