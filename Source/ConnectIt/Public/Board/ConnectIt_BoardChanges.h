// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"


// --- What one step of a board change did ---------------------------------
// FConnectItBoardOperation::Apply reports what it really did here; the rule
// set's after-move step (FConnectItRuleSet::ResolveBoardChange) scores from
// it, and the scoring pass adds what scoring did. Tile powers react to it
// (see the tile-powers plan): it is what a power's trigger reads -- the AI's
// search has no event list, so this record is always produced.
//
// Plain data with inline storage: built for every position the AI's search
// tries, so an ordinary move allocates nothing.
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
};

// A piece that left Position (not cleared by scoring)
struct FConnectItPieceExit
{
    FGridPosition Position;
    int32 Faction = INDEX_NONE;
};

// The piece on Position changed owner (a capture)
struct FConnectItPieceChange
{
    FGridPosition Position;
    int32 Faction = INDEX_NONE;
    int32 PreviousFaction = INDEX_NONE;
    bool bSurvivesScoring = true;
};

// What about a tile changed
enum class EConnectItTileChange : uint8
{
    Multiplier,
    Active,
    Power,
};

struct FConnectItTileChange
{
    FGridPosition Position;
    EConnectItTileChange What = EConnectItTileChange::Multiplier;

    // Whose change it was (the operation's faction, or the scoring faction
    // for a multiplier scoring raised); INDEX_NONE for nobody
    int32 Faction = INDEX_NONE;
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

    // A tile's own data changed (multiplier, active, power). Sets off Tile
    // Changed powers.
    TArray<FConnectItTileChange, TInlineAllocator<4>> TileChanged;

    // The positions a scoring pass's lines ran through (each once): where
    // Scoring powers fire. Only a scoring pass fills it.
    TArray<FGridPosition, TInlineAllocator<8>> ScoredThrough;

    // Whose change this step is (the operation's faction); INDEX_NONE for
    // nobody
    int32 Faction = INDEX_NONE;

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

    void AddTileChanged(const FGridPosition& Position, EConnectItTileChange What, int32 ChangeFaction)
    {
        TileChanged.Add({ Position, What, ChangeFaction });
    }

    // Every piece that may score after this step, in order: arrivals, owner
    // changes, then moved pieces
    template<typename TAllocator>
    void GetScoringArrivals(TArray<FConnectItPieceArrival, TAllocator>& OutArrivals) const
    {
        OutArrivals.Append(Entered);
        for (const FConnectItPieceChange& Change : PieceChanged)
        {
            OutArrivals.Add({ Change.Position, Change.Faction, Change.bSurvivesScoring });
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
    }
};

// The arrivals a scoring pass looks at (inline: most passes have one)
using FConnectItScoringArrivals = TArray<FConnectItPieceArrival, TInlineAllocator<8>>;
