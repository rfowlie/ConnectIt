// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/Rules/ConnectIt_LineScoringRule.h"
#include "Board/Events/ConnectIt_BoardEvents.h"


float FConnectItScoringRule_Lines::ApplyScoring(
    FConnectItBoardState& MutableState,
    FGridPosition Position,
    int32 FactionSlot,
    bool bArrivingPieceSurvives,
    FConnectItBoardChangeEvent* OutEvents) const
{
    TArray<TArray<FGridPosition>> ScoringLines =
        FindScoringLines(MutableState, Position, FactionSlot, ConnectLength);

    if (ScoringLines.IsEmpty()) return 0.f;

    float TotalPoints = 0.f;

    for (const TArray<FGridPosition>& Line : ScoringLines)
    {
        TArray<FGridPosition> ClearedPositions;
        const float LinePoints = ApplyScoringLine(
            MutableState, Line, Position, bArrivingPieceSurvives,
            OutEvents ? &ClearedPositions : nullptr);
        TotalPoints += LinePoints;

        // One Scored event per completed line (lines always share Position)
        if (OutEvents)
        {
            FConnectItBoardEvent_Scored Event;
            Event.Faction   = FactionSlot;
            Event.Points    = LinePoints;
            Event.Positions = Line;
            Event.ClearedPositions = MoveTemp(ClearedPositions);
            OutEvents->Add(Event);
        }
    }

    if (MutableState.ScoreBoard.IsValidIndex(FactionSlot))
    {
        MutableState.ScoreBoard[FactionSlot] += TotalPoints;
    }

    return TotalPoints;
}

TArray<TArray<FGridPosition>> FConnectItScoringRule_Lines::FindScoringLines(
    const FConnectItBoardState& State,
    FGridPosition Position,
    int32 FactionSlot,
    int32 ConnectLength)
{
    TArray<TArray<FGridPosition>> ScoringLines;

    for (const FGridDirectionVector& Dir : GetScoringDirections())
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
    FGridPosition CompletingPosition,
    bool bKeepCompletingPiece,
    TArray<FGridPosition>* OutClearedPositions)
{
    float PointsScored = 0.f;

    for (const FGridPosition& Position : Line)
    {
        FConnectItTileData* TileData = MutableState.GetTileDataMutable(Position);
        if (!TileData) continue;

        // Every tile of the line scores and gains multiplier...
        PointsScored += TileData->Multiplier;
        TileData->Multiplier += 1.0f;

        // ...and loses its piece, except the completing piece when it is kept
        if (bKeepCompletingPiece && Position == CompletingPosition) continue;

        // Already empty when an earlier line through the same completing
        // piece cleared it
        if (TileData->FactionPiece != -1)
        {
            TileData->SetFactionPiece(-1);
            if (OutClearedPositions) OutClearedPositions->Add(Position);
        }
    }

    return PointsScored;
}

const TArray<FGridDirectionVector>&
FConnectItScoringRule_Lines::GetScoringDirections()
{
    static const TArray<FGridDirectionVector> Directions =
    {
        { 0,  1 },  // Row
        { 1,  0 },  // Column
        { 1,  1 },  // Diagonal top-down
        { 1, -1 }   // Diagonal bottom-up
    };
    return Directions;
}
