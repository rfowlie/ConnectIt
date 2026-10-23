// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"

struct FConnectItRuleSet;
struct FConnectItTileData;


// --- What one step of a board change did ---------------------------------
// FConnectItBoardOperation::Apply reports what it really did here; the rule
// set's after-move step (FConnectItRuleSet::ResolveBoardChange) scores from
// it, and the scoring pass adds what scoring did. Tile powers react to it
// (see the tile-powers plan): it is what a power's trigger reads -- the AI's
// search has no event list, so this record is always produced.
//
// Plain data with inline storage: built for every position the AI's search
// tries, so an ordinary move allocates nothing.
//
// Each entry's Cause is the index (in the change's event list) of the event
// that brought it about -- a tile power that fires on it records that as its
// own event's CauseIndex. INDEX_NONE: the action itself, or no event list (the
// AI's search).
// ----------------------------------------------------------------------------

// A piece now on Position that may score
struct FConnectItPieceArrival
{
    FGridPosition Position;

    // The piece's faction
    int32 Faction = INDEX_NONE;

    // Whether it stays on its tile if it completes something that scores --
    // the answer of the operation that brought it
    // (FConnectItBoardOperation::ArrivingPiecesSurviveScoring)
    bool bSurvivesScoring = true;

    int32 Cause = INDEX_NONE;
};

// A piece that left Position (not cleared by scoring)
struct FConnectItPieceExit
{
    FGridPosition Position;
    int32 Faction = INDEX_NONE;
    int32 Cause = INDEX_NONE;
};

// The piece on Position changed owner (a capture)
struct FConnectItPieceChange
{
    FGridPosition Position;
    int32 Faction = INDEX_NONE;
    int32 PreviousFaction = INDEX_NONE;
    bool bSurvivesScoring = true;
    int32 Cause = INDEX_NONE;
};

// What about a tile changed
enum class EConnectItTileChange : uint8
{
    Multiplier,
    Active,
    Power,
    Lock,
};

struct FConnectItTileChange
{
    FGridPosition Position;
    EConnectItTileChange What = EConnectItTileChange::Multiplier;

    // Whose change it was (the operation's faction, or the scoring faction
    // for a multiplier scoring raised); INDEX_NONE for nobody
    int32 Faction = INDEX_NONE;

    int32 Cause = INDEX_NONE;
};

// A tile a scoring pass's line ran through, and whose line it was
struct FConnectItScoredTile
{
    FGridPosition Position;
    int32 Faction = INDEX_NONE;
    int32 Cause = INDEX_NONE;
};

struct FConnectItBoardChanges
{
    // A piece arrived on a tile: placed, swapped in. Scored; sets off Piece
    // Enter powers.
    TArray<FConnectItPieceArrival, TInlineAllocator<4>> Entered;

    // A piece left a tile: removed, swapped out. Sets off Piece Exit powers.
    TArray<FConnectItPieceExit, TInlineAllocator<4>> Exited;

    // A tile got a piece without one arriving on it: a shift (the tile and its
    // piece moved together), a tile added with its starting piece. Scored;
    // sets off nothing.
    TArray<FConnectItPieceArrival, TInlineAllocator<8>> Moved;

    // A piece changed owner. Scored like an arrival; sets off Piece Changed
    // powers.
    TArray<FConnectItPieceChange, TInlineAllocator<2>> PieceChanged;

    // A tile's own data changed (multiplier, active, power, lock). Sets off Tile
    // Changed powers.
    TArray<FConnectItTileChange, TInlineAllocator<4>> TileChanged;

    // The tiles a scoring pass's lines ran through (each once), with the
    // scoring faction: where Scoring powers fire. Only a scoring pass fills it.
    TArray<FConnectItScoredTile, TInlineAllocator<8>> ScoredThrough;

    // Whose change this step is (the operation's faction); INDEX_NONE for
    // nobody
    int32 Faction = INDEX_NONE;

    // A turn started (the Turn Start operation): the step's settle fires the
    // Turn Start tile powers whose countdown has run out
    bool bTurnStarted = false;

    // The rules the change is carried out under, set by whoever carries it out
    // (CarryOutOperation, the scoring pass, the power chain, the AI's
    // ApplyMove), so an operation or the scoring rule can ask them -- e.g. a
    // tile type's multiplier limits (ClampMultiplier). Null: defaults.
    const FConnectItRuleSet* Rules = nullptr;

    // Value within the multiplier limits of Tile's type, from Rules (no
    // rules: a plain tile's -- floor 1, no ceiling)
    float ClampMultiplier(const FConnectItTileData& Tile, float Value) const;

    void AddEntered(const FGridPosition& Position, int32 PieceFaction, bool bSurvivesScoring)
    {
        Entered.Add({ Position, PieceFaction, bSurvivesScoring });
    }

    void AddExited(const FGridPosition& Position, int32 PieceFaction)
    {
        Exited.Add({ Position, PieceFaction });
    }

    void AddMoved(const FGridPosition& Position, int32 PieceFaction, bool bSurvivesScoring)
    {
        Moved.Add({ Position, PieceFaction, bSurvivesScoring });
    }

    void AddPieceChanged(const FGridPosition& Position, int32 NewFaction, int32 PreviousFaction, bool bSurvivesScoring)
    {
        PieceChanged.Add({ Position, NewFaction, PreviousFaction, bSurvivesScoring });
    }

    void AddTileChanged(const FGridPosition& Position, EConnectItTileChange What, int32 ChangeFaction,
        int32 Cause = INDEX_NONE)
    {
        TileChanged.Add({ Position, What, ChangeFaction, Cause });
    }

    // Whether a scoring pass ran through Position
    bool WasScoredThrough(const FGridPosition& Position) const
    {
        return ScoredThrough.ContainsByPredicate([&Position](const FConnectItScoredTile& Tile) { return Tile.Position == Position; });
    }

    // Adds every entry of Other (its faction is left as it is)
    void Append(const FConnectItBoardChanges& Other)
    {
        Entered.Append(Other.Entered);
        Exited.Append(Other.Exited);
        Moved.Append(Other.Moved);
        PieceChanged.Append(Other.PieceChanged);
        TileChanged.Append(Other.TileChanged);
        ScoredThrough.Append(Other.ScoredThrough);
    }

    // Every entry was brought about by the event at Cause
    void SetCause(int32 Cause)
    {
        for (FConnectItPieceArrival& Entry : Entered) Entry.Cause = Cause;
        for (FConnectItPieceExit& Entry : Exited) Entry.Cause = Cause;
        for (FConnectItPieceArrival& Entry : Moved) Entry.Cause = Cause;
        for (FConnectItPieceChange& Entry : PieceChanged) Entry.Cause = Cause;
        for (FConnectItTileChange& Entry : TileChanged) Entry.Cause = Cause;
        for (FConnectItScoredTile& Entry : ScoredThrough) Entry.Cause = Cause;
    }

    // Every piece that may score after this step, in order: arrivals, owner
    // changes, then moved pieces
    template<typename TAllocator>
    void GetScoringArrivals(TArray<FConnectItPieceArrival, TAllocator>& OutArrivals) const
    {
        OutArrivals.Append(Entered);
        for (const FConnectItPieceChange& Change : PieceChanged)
        {
            OutArrivals.Add({ Change.Position, Change.Faction, Change.bSurvivesScoring, Change.Cause });
        }
        OutArrivals.Append(Moved);
    }

    bool HasScoringArrivals() const
    {
        return !Entered.IsEmpty() || !PieceChanged.IsEmpty() || !Moved.IsEmpty();
    }

    bool IsEmpty() const
    {
        return !HasScoringArrivals() && Exited.IsEmpty() && TileChanged.IsEmpty() && ScoredThrough.IsEmpty();
    }

    void Reset()
    {
        Entered.Reset();
        Exited.Reset();
        Moved.Reset();
        PieceChanged.Reset();
        TileChanged.Reset();
        ScoredThrough.Reset();
        Faction = INDEX_NONE;
        bTurnStarted = false;
    }
};

// The arrivals a scoring pass looks at (inline: most passes have one)
using FConnectItScoringArrivals = TArray<FConnectItPieceArrival, TInlineAllocator<8>>;
