// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "ConnectIt_LevelChange.generated.h"


// When a change made from outside a move (a director replacing a loadout, the
// level changing the board, the scoring rule or a side's goals) takes effect.
// (Formerly EConnectItLoadoutTiming.)
UENUM(BlueprintType)
enum class EConnectItChangeTiming : uint8
{
    // When the next turn begins (for a loadout: when that side's next turn
    // begins). The default for loadouts: re-seeding a loadout mid-turn would
    // hand out extra actions in a turn already under way.
    NextTurnStart   UMETA(DisplayName = "Next Turn Start"),

    // Now -- or, while a board change is resolving (being carried out, or its
    // visuals still playing; directors' reactions to a change run inside it),
    // once that change has played out, never part-way through it. Several
    // play one after another, each with its own visuals. (A loadout asked
    // for during a move waits for that side's next turn instead: the move's
    // action use is spent against the loadout it came from.)
    Immediately     UMETA(DisplayName = "Immediately")
};

class UConnectIt_MatchConditionAsset;
class UConnectIt_BoardLayout;

// A change the level makes outside any player's move: a board operation, a
// new scoring rule, a side's new goals, or a new layout (exactly one is set)
struct FConnectItLevelChange
{
    // A board operation struct (e.g. Force Place Piece), made by AsFaction
    // (INDEX_NONE for one that belongs to nobody, e.g. Toggle Tile Active)
    FInstancedStruct Operation;
    int32 AsFaction = INDEX_NONE;

    // A scoring rule struct (e.g. Lines) replacing the match's
    FInstancedStruct ScoringRule;

    // A side's goals replaced (bHasGoals): the AI's or the human side's, win
    // goals or lose conditions. Weak: this queue isn't seen by the garbage
    // collector (the assets are held by the Level director that asked).
    bool bHasGoals = false;
    bool bGoalsForAI = false;
    bool bWinGoals = true;
    TArray<TWeakObjectPtr<UConnectIt_MatchConditionAsset>> Goals;

    // A layout the board's tiles change to (bHasLayout). Kept as the asset,
    // not as a ready-made operation: the change is worked out against the
    // board as it is when this is applied, not when it was asked for. Weak,
    // as above.
    bool bHasLayout = false;
    TWeakObjectPtr<UConnectIt_BoardLayout> Layout;
};

// The GameMode's level changes that can't be applied yet: those waiting for
// the board to settle (applied one at a time, in order, as each board change
// plays out) and those waiting for the next turn to start
class CONNECTIT_API FConnectItLevelChangeQueue
{
public:

    // Queues Change for Timing, or returns true when it should be applied now:
    // Immediately, nothing resolving (bBusy false) and nothing already waiting
    // ahead of it
    bool Add(const FConnectItLevelChange& Change, EConnectItChangeTiming Timing, bool bBusy);

    // A turn has started: the changes waiting for it join the back of the
    // waiting line
    void StartTurn();

    // Change goes to the front of the waiting line (the turn's start comes
    // before anything queued for it)
    void AddFirst(const FConnectItLevelChange& Change) { Waiting.Insert(Change, 0); }

    // The next waiting change, removed; false if none
    bool TakeNext(FConnectItLevelChange& OutChange);

    // Every waiting change dropped (not those for the next turn); returns how many
    int32 DiscardWaiting();

    bool HasWaiting() const { return !Waiting.IsEmpty(); }
    int32 NumWaiting() const { return Waiting.Num(); }
    int32 NumForNextTurn() const { return NextTurn.Num(); }

private:

    TArray<FConnectItLevelChange> Waiting;
    TArray<FConnectItLevelChange> NextTurn;
};
