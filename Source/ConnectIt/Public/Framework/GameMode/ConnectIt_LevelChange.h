// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "ConnectIt_LevelChange.generated.h"


// When a change made from outside a move (a director replacing a loadout, the
// level changing the board or the scoring rule) takes effect.
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

// A change the level makes outside any player's move: a board operation, or a
// new scoring rule (exactly one is set)
struct FConnectItLevelChange
{
    // A board operation struct (e.g. Force Place Piece), made by AsFaction
    // (INDEX_NONE for one that belongs to nobody, e.g. Toggle Tile Active)
    FInstancedStruct Operation;
    int32 AsFaction = INDEX_NONE;

    // A scoring rule struct (e.g. Lines) replacing the match's
    FInstancedStruct ScoringRule;
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
