// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/Rules/ConnectIt_LineScoringRule.h"


float FConnectItScoringRule_Lines::ApplyScoring(
    FConnectItBoardState& MutableState,
    FGridPosition Position,
    int32 FactionSlot,
    TArray<FConnectItScoringConfiguration>* OutConfigurations) const
{
    TArray<TArray<FGridPosition>> ScoringLines =
        FindScoringLines(MutableState, Position, FactionSlot, ConnectLength);

    if (ScoringLines.IsEmpty()) return 0.f;

    float TotalPoints = 0.f;

    for (const TArray<FGridPosition>& Line : ScoringLines)
    {
        const float LinePoints = ApplyScoringLine(
            MutableState, Line, Position, FactionSlot);
        TotalPoints += LinePoints;

        // One configuration per completed line (lines always share Position)
        if (OutConfigurations)
        {
            FConnectItScoringConfiguration& Configuration = OutConfigurations->AddDefaulted_GetRef();
            Configuration.FactionSlot = FactionSlot;
            Configuration.Points      = LinePoints;
            Configuration.Positions   = Line;
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
    int32 FactionSlot)
{
    float PointsScored = 0.f;

    for (const FGridPosition& Position : Line)
    {
        FConnectItTileData* TileData = MutableState.GetTileDataMutable(Position);
        if (!TileData) continue;

        PointsScored += TileData->Multiplier;

        // Remove piece and increment multiplier
        TileData->SetFactionPiece(-1);
        TileData->Multiplier  += 1.0f;
    }

    // Completing piece stays on the board
    if (FConnectItTileData* CompletingTile =
        MutableState.GetTileDataMutable(CompletingPosition))
    {
        CompletingTile->SetFactionPiece(FactionSlot);
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
