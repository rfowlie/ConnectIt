// Fill out your copyright notice in the Description page of Project Settings.

#include "MinMax/ConnectIt_MinMaxTerms.h"
#include "MinMax/ConnectIt_MinMaxRules.h"


// --- Evaluation ---

float FConnectItMinMaxEvalTerm_ScoreDifference::Evaluate(
    const FConnectItMinMaxRules& Rules, const FConnectItBoardState& Board, int32 Side) const
{
    return Board.GetScore(Side) - Board.GetScore(FConnectItMinMaxRules::Opponent(Side));
}

float FConnectItMinMaxEvalTerm_LinePotential::Evaluate(
    const FConnectItMinMaxRules& Rules, const FConnectItBoardState& Board, int32 Side) const
{
    return LinePotential(Rules, Board, Side)
        - LinePotential(Rules, Board, FConnectItMinMaxRules::Opponent(Side));
}

float FConnectItMinMaxEvalTerm_LinePotential::LinePotential(
    const FConnectItMinMaxRules& Rules, const FConnectItBoardState& Board, int32 Side)
{
    float Potential = 0.f;

    for (const TArray<int32>& Window : Rules.GetGeometry().LineWindows)
    {
        int32 Pieces = 0;
        float Multiplier = 0.f;
        bool bOpen = true;

        for (const int32 Index : Window)
        {
            const FConnectItTileData& Tile = Board.GetTileDataAt(Index);
            if (Tile.FactionPiece == Side)
            {
                Pieces++;
            }
            else if (!Tile.bIsActive || Tile.bIsOccupied)
            {
                // Inactive, the other faction's piece, or a non-faction blocker
                bOpen = false;
                break;
            }

            Multiplier += Tile.Multiplier;
        }

        if (bOpen && Pieces > 0)
        {
            Potential += static_cast<float>(Pieces * Pieces) * Multiplier;
        }
    }

    return Potential;
}

// --- Move ordering ---

float FConnectItMinMaxOrderTerm_TileMultiplier::Score(
    const FConnectItMinMaxRules& Rules, const FConnectItBoardState& Board,
    int32 TileIndex, int32 Side) const
{
    return Board.GetTileDataAt(TileIndex).Multiplier;
}

float FConnectItMinMaxOrderTerm_AdjacentPieces::Score(
    const FConnectItMinMaxRules& Rules, const FConnectItBoardState& Board,
    int32 TileIndex, int32 Side) const
{
    const TArray<TArray<int32>>& Neighbours = Rules.GetGeometry().Neighbours;
    if (!Neighbours.IsValidIndex(TileIndex)) return 0.f;

    int32 Count = 0;
    for (const int32 NeighbourIndex : Neighbours[TileIndex])
    {
        if (Board.GetTileDataAt(NeighbourIndex).FactionPiece != -1)
        {
            Count++;
        }
    }
    return static_cast<float>(Count);
}
