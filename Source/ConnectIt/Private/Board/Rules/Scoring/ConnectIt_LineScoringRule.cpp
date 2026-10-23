// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/Rules/Scoring/ConnectIt_LineScoringRule.h"
#include "Board/Events/ConnectIt_BoardEvents.h"


namespace
{
    // One line a pass found: whose, where, and the piece it keeps (if any)
    struct FFoundLine
    {
        TArray<FGridPosition> Positions;
        int32 Faction = INDEX_NONE;
        bool bHasKeeper = false;
        FGridPosition Keeper;

        // The event that brought the arrival that found it (its Scored event's cause)
        int32 Cause = INDEX_NONE;
    };

    // The same line (same faction, same tiles in any order)
    bool IsSameLine(const FFoundLine& Line, int32 Faction, const TArray<FGridPosition>& Positions)
    {
        if (Line.Faction != Faction || Line.Positions.Num() != Positions.Num()) return false;
        for (const FGridPosition& Position : Positions)
        {
            if (!Line.Positions.Contains(Position)) return false;
        }
        return true;
    }
}

float FConnectItScoringRule_Lines::ScorePass(
    FConnectItBoardState& MutableState,
    TConstArrayView<FConnectItPieceArrival> Arrivals,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    // 1. Find: every line through every arrival, on the board as it is --
    //    nothing is cleared until all are found
    TArray<FFoundLine, TInlineAllocator<4>> Found;
    for (const FConnectItPieceArrival& Arrival : Arrivals)
    {
        // The piece there now (an earlier step may have changed it); an empty
        // tile or a non-faction blocker can't score
        const FConnectItTileData* Tile = MutableState.GetTileData(Arrival.Position);
        if (!Tile || Tile->FactionPiece == INDEX_NONE) continue;
        const int32 Faction = Tile->FactionPiece;

        for (TArray<FGridPosition>& Line : FindScoringLines(MutableState, Arrival.Position, Faction))
        {
            FFoundLine* Existing = Found.FindByPredicate([Faction, &Line](const FFoundLine& Candidate)
            {
                return IsSameLine(Candidate, Faction, Line);
            });

            // A line two arrivals share counts once; its kept piece is the
            // first arrival on it that survives scoring
            if (Existing)
            {
                if (!Existing->bHasKeeper && Arrival.bSurvivesScoring)
                {
                    Existing->bHasKeeper = true;
                    Existing->Keeper = Arrival.Position;
                }
                continue;
            }

            FFoundLine& Added = Found.AddDefaulted_GetRef();
            Added.Positions = MoveTemp(Line);
            Added.Faction = Faction;
            Added.bHasKeeper = Arrival.bSurvivesScoring;
            Added.Keeper = Arrival.Position;
            Added.Cause = Arrival.Cause;
        }
    }

    if (Found.IsEmpty()) return 0.f;

    // A piece one line keeps is never cleared by another line through it
    TArray<FGridPosition, TInlineAllocator<4>> Kept;
    for (const FFoundLine& Line : Found)
    {
        if (Line.bHasKeeper) Kept.AddUnique(Line.Keeper);
    }

    // 2. Apply them all, in the order found
    float TotalPoints = 0.f;
    for (const FFoundLine& Line : Found)
    {
        TArray<FGridPosition> ClearedPositions;
        const float LinePoints = ApplyScoringLine(
            MutableState, Line.Positions, Kept, OutEvents ? &ClearedPositions : nullptr, OutChanges);
        TotalPoints += LinePoints;

        if (MutableState.ScoreBoard.IsValidIndex(Line.Faction))
        {
            MutableState.ScoreBoard[Line.Faction] += LinePoints;
        }

        // What scoring did, for what follows (each tile once), caused by this
        // line's Scored event
        const int32 ScoredEventIndex = OutEvents ? OutEvents->Events.Num() : INDEX_NONE;
        for (const FGridPosition& Position : Line.Positions)
        {
            if (!OutChanges.WasScoredThrough(Position))
            {
                OutChanges.ScoredThrough.Add({ Position, Line.Faction, ScoredEventIndex });
                OutChanges.AddTileChanged(Position, EConnectItTileChange::Multiplier, Line.Faction, ScoredEventIndex);
            }
        }

        // One Scored event per completed line
        if (OutEvents)
        {
            FConnectItBoardEvent_Scored Event;
            Event.CauseIndex = Line.Cause;
            Event.Faction   = Line.Faction;
            Event.Points    = LinePoints;
            Event.Positions = Line.Positions;
            Event.ClearedPositions = MoveTemp(ClearedPositions);
            Event.NewMultipliers.Reserve(Line.Positions.Num());
            for (const FGridPosition& Position : Line.Positions)
            {
                const FConnectItTileData* Tile = MutableState.GetTileData(Position);
                Event.NewMultipliers.Add(Tile ? Tile->Multiplier : 1.f);
            }
            OutEvents->Add(Event);
        }
    }

    return TotalPoints;
}

TArray<TArray<FGridPosition>> FConnectItScoringRule_Lines::FindScoringLines(
    const FConnectItBoardState& State,
    FGridPosition Position,
    int32 FactionSlot) const
{
    TArray<TArray<FGridPosition>> ScoringLines;

    // Only lines along an allowed direction count
    for (const FGridDirectionVector& Dir : GetAllowedAxes())
    {
        TArray<FGridPosition> Line;
        Line.Add(Position);

        // Walk positive direction
        for (int32 Step = 1; Step < ConnectLength * 2; Step++)
        {
            FGridPosition Check(
                Position.X + Step * Dir.Row,
                Position.Y + Step * Dir.Column);

            const FConnectItTileData* Data = State.GetTileData(Check);
            if (!Data || Data->FactionPiece != FactionSlot) break;
            Line.Add(Check);
        }

        // Walk negative direction
        for (int32 Step = 1; Step < ConnectLength * 2; Step++)
        {
            FGridPosition Check(
                Position.X - Step * Dir.Row,
                Position.Y - Step * Dir.Column);

            const FConnectItTileData* Data = State.GetTileData(Check);
            if (!Data || Data->FactionPiece != FactionSlot) break;
            Line.Add(Check);
        }

        if (Line.Num() >= ConnectLength)
        {
            ScoringLines.Add(MoveTemp(Line));
        }
    }

    return ScoringLines;
}

float FConnectItScoringRule_Lines::ApplyScoringLine(
    FConnectItBoardState& MutableState,
    const TArray<FGridPosition>& Line,
    TConstArrayView<FGridPosition> KeptPositions,
    TArray<FGridPosition>* OutClearedPositions,
    const FConnectItBoardChanges& Changes)
{
    float PointsScored = 0.f;

    for (const FGridPosition& Position : Line)
    {
        FConnectItTileData* TileData = MutableState.GetTileDataMutable(Position);
        if (!TileData) continue;

        // Every tile of the line scores and gains multiplier...
        PointsScored += TileData->Multiplier;
        TileData->Multiplier = Changes.ClampMultiplier(*TileData, TileData->Multiplier + 1.0f);

        // ...and loses its piece, except a kept piece
        if (KeptPositions.Contains(Position)) continue;

        // Already empty when an earlier line of the pass cleared it
        if (TileData->FactionPiece != -1)
        {
            TileData->SetFactionPiece(-1);
            if (OutClearedPositions) OutClearedPositions->Add(Position);
        }
    }

    return PointsScored;
}

FConnectItScoringRule_Lines::FAxes FConnectItScoringRule_Lines::GetAllowedAxes() const
{
    FAxes Axes;
    if (IsDirectionAllowed(EConnectItScoringDirection::Horizontal))
    {
        Axes.Add({ 0, 1 });     // Row
    }
    if (IsDirectionAllowed(EConnectItScoringDirection::Vertical))
    {
        Axes.Add({ 1, 0 });     // Column
    }
    if (IsDirectionAllowed(EConnectItScoringDirection::Diagonal))
    {
        Axes.Add({ 1, 1 });     // Diagonal top-down
        Axes.Add({ 1, -1 });    // Diagonal bottom-up
    }
    return Axes;
}
