// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/ConnectIt_BoardChanges.h"
#include "Board/Powers/ConnectIt_TilePower.h"

struct FConnectItBoardState;
struct FConnectItBoardChangeEvent;
struct FConnectItRuleSet;


// One action's tile powers, for FConnectItRuleSet::ResolveBoardChange: the two
// places powers fire (settle, before each scoring pass; scoring reactions,
// after one) and the once-per-action guard that ends every chain. Lives for
// one ResolveBoardChange call; runs wherever it does (the server, Simulate
// Operation, the AI's search), so plain data only.
//
// Firing a power: its gate (the tile's multiplier) and cooldown (the board's
// turn) must allow it, and -- unless it is Sync -- it must not have fired on
// that tile this action. It appends Power Triggered, starts its cooldown, asks
// its effect for operations, and applies each one the rules allow
// (FConnectItRuleSet::CanCarryOut -- so not on a locked tile unless the
// operation may). It applies them with Apply, not CarryOutOperation: what
// they change goes back into this chain, not a new one. They act for the
// faction of the change that set the power off.
class FConnectItPowerChain
{
public:

    FConnectItPowerChain(const FConnectItRuleSet& InRules, FConnectItBoardState& InBoard, FConnectItBoardChangeEvent* InEvents);

    // Whether the match has any tile powers (none: the chain is just scoring)
    bool HasPowers() const { return bHasPowers; }

    // Settle, before a scoring pass: on a turn's start, its Turn Start powers
    // first (once); then Sync powers, then the Piece Enter,
    // Piece Exit, Piece Changed and Tile Changed powers InOutStep sets off (in
    // that order, in the order the record lists them); then again on what
    // those changed, until nothing more fires. Everything they changed is
    // added to InOutStep (so it is scored).
    void Settle(FConnectItBoardChanges& InOutStep);

    // After a scoring pass: the Scoring powers on the tiles its lines ran
    // through, in order, acting for the scoring faction. What they changed is
    // added to InOutScored (which goes back through settle).
    void ReactToScoring(FConnectItBoardChanges& InOutScored);

private:

    // The tile's own power and each level-wide power whose trigger is
    // Trigger, on Position. True if any fired.
    bool FireAt(const FGridPosition& Position, EConnectItPowerTrigger Trigger, int32 Faction, int32 Cause,
        FConnectItBoardChanges& OutChanges);

    // The powers Triggers sets off. True if any fired.
    bool FireTriggers(const FConnectItBoardChanges& Triggers, FConnectItBoardChanges& OutChanges);

    // Every Sync power, on every tile it applies to. True if any changed something.
    bool RunSyncPowers(int32 Faction, FConnectItBoardChanges& OutChanges);

    // Every Turn Start power whose countdown has run out, on every tile it
    // applies to. True if any fired.
    bool RunTurnStartPowers(int32 Faction, FConnectItBoardChanges& OutChanges);

    // One power on one tile (LevelWideIndex: which level-wide power, or
    // INDEX_NONE for the tile's own). True if it fired (a Sync power: if it
    // changed something).
    bool TryFire(const FGridPosition& Position, const FConnectItTilePower& Power, int32 LevelWideIndex,
        int32 Faction, int32 Cause, FConnectItBoardChanges& OutChanges);

    const FConnectItRuleSet& Rules;
    FConnectItBoardState& Board;
    FConnectItBoardChangeEvent* Events;

    // The level-wide powers, looked up once (null where the rules don't define one)
    TArray<const FConnectItTilePower*, TInlineAllocator<4>> LevelWide;

    bool bHasPowers = false;
    bool bHasSyncPowers = false;

    // The once-per-action guard: (tile, power slot) pairs that have fired.
    // A tile is its id (it survives a shift), or its position for a board
    // built without ids; slot 0 is the tile's own power, 1 + i level-wide i.
    TArray<TPair<int64, int32>, TInlineAllocator<16>> Fired;
};
