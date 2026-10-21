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

    // Now -- or, while a move is being carried out (directors' reactions to a
    // board change run inside it), right after that move has finished, never
    // part-way through its scoring. (A loadout asked for during a move waits
    // for that side's next turn instead: the move's action use is spent
    // against the loadout it came from.)
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

// The GameMode's waiting level changes: what is applied right after the move
// being carried out, and what waits for the next turn to start
class CONNECTIT_API FConnectItLevelChangeQueue
{
public:

    // Queues Change for Timing, or returns true when it should be applied now
    // (Immediately, and nothing is being carried out: bBusy false)
    bool Add(const FConnectItLevelChange& Change, EConnectItChangeTiming Timing, bool bBusy);

    // The changes waiting for the current move to finish, emptied
    TArray<FConnectItLevelChange> TakeAfterMove();

    // The changes waiting for the next turn, emptied
    TArray<FConnectItLevelChange> TakeNextTurn();

    int32 NumWaiting() const { return AfterMove.Num() + NextTurn.Num(); }

private:

    TArray<FConnectItLevelChange> AfterMove;
    TArray<FConnectItLevelChange> NextTurn;
};
