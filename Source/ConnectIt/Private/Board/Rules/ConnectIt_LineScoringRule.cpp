// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/Rules/ConnectIt_LineScoringRule.h"


float UConnectIt_LineScoringRule::ApplyScoring_Implementation(
    FConnectItBoardState& MutableState,
    FGridPosition Position,
    int32 FactionSlot)
{
    TArray<TArray<FGridPosition>> ScoringLines =
        FindScoringLines(MutableState, Position, FactionSlot);

    if (ScoringLines.IsEmpty()) return 0.f;

    float TotalPoints = 0.f;

    for (const TArray<FGridPosition>& Line : ScoringLines)
    {
        TotalPoints += ApplyScoringLine(
            MutableState, Line, Position, FactionSlot);
    }

    if (MutableState.ScoreBoard.IsValidIndex(FactionSlot))
    {
        MutableState.ScoreBoard[FactionSlot] += TotalPoints;
    }

    return TotalPoints;
}

TArray<TArray<FGridPosition>> UConnectIt_LineScoringRule::FindScoringLines(
    const FConnectItBoardState& State,
    FGridPosition Position,
    int32 FactionSlot) const
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

float UConnectIt_LineScoringRule::ApplyScoringLine(
    FConnectItBoardState& MutableState,
    const TArray<FGridPosition>& Line,
    FGridPosition CompletingPosition,
    int32 FactionSlot) const
{
    float PointsScored = 0.f;

    for (const FGridPosition& Position : Line)
    {
        FConnectItTileData* TileData = MutableState.GetTileDataMutable(Position);
        if (!TileData) continue;

        PointsScored += TileData->Multiplier;

        // Remove piece and increment multiplier
        TileData->FactionPiece = -1;
        TileData->Multiplier  += 1.0f;
    }

    // Completing piece stays on the board
    if (FConnectItTileData* CompletingTile =
        MutableState.GetTileDataMutable(CompletingPosition))
    {
        CompletingTile->FactionPiece = FactionSlot;
    }

    return PointsScored;
}

const TArray<FGridDirectionVector>&
UConnectIt_LineScoringRule::GetScoringDirections()
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
