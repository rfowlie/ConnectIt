// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "GridMechanicsBaseStructs.h"
#include "StructUtils/InstancedStruct.h"
#include "MinMax/ConnectIt_MinMaxTerms.h"
#include "Search/MinMax/GI_MinMaxAlphaBeta.h"


// Board geometry for one decision: depends only on tile positions and the
// connect length, never on pieces, so it's built once (game thread) and read
// by every node of the search.
struct FConnectItMinMaxGeometry
{
    // Every run of ConnectLength existing tiles along one of the scoring
    // axes (UConnectIt_LineScoringRule::GetScoringDirections), as indices
    // into FConnectItBoardState::TileDataArray.
    TArray<TArray<int32>> LineWindows;

    // Each tile's existing neighbours (up to 8), as TileDataArray indices.
    TArray<TArray<int32>> Neighbours;

    static FConnectItMinMaxGeometry Build(const FConnectItBoardState& Board, int32 ConnectLength);
};

// Classic ConnectIt -- two factions, one PlacePiece per turn, nothing else --
// as the MinMax search (GameIntelligence::Search::MinMax::TAlphaBeta) sees it.
//
// Not something you configure or subclass: UConnectIt_AIStrategy_MinMax
// builds one per decision, on the game thread, from the live board and its
// own editor data, then the search only reads it. The fixed part is the game
// model (which moves exist, what a move does, when the game is over, and the
// win/loss value); what the AI VALUES comes from the strategy's evaluation and
// ordering terms (see ConnectIt_MinMaxTerms.h).
//
// Thread safety: the search runs on a background task, so nothing here may
// touch a UObject. A simulated move is scored with
// UConnectIt_LineScoringRule::ApplyLineScoring -- the exact static algorithm
// the real scoring rule runs, never its BlueprintNativeEvent interface (which
// could be Blueprint-authored and must stay on the game thread).
class CONNECTIT_API FConnectItMinMaxRules final
{
public:

    struct FState
    {
        FConnectItBoardState Board;

        // Faction slot (0 or 1) about to place. Evaluate scores for this side.
        int32 SideToMove = 0;
    };

    struct FMove
    {
        int32 TileIndex = INDEX_NONE;
        FGridPosition Position;
    };

    // Win/loss score; a win found Ply plies from the root is worth
    // WinValue - Ply, so nearer wins (and farther losses) are preferred.
    static constexpr int32 WinValue = 10000000;

    // Board is only used for its geometry -- the position to search from is
    // passed to MakeRoot. WinThreshold 0 = the win condition isn't
    // score-based, so the search never sees a win. The term arrays are
    // copied; invalid entries and zero-weight terms are skipped.
    FConnectItMinMaxRules(
        const FConnectItBoardState& Board,
        int32 InConnectLength,
        float InWinThreshold,
        const TArray<TInstancedStruct<FConnectItMinMaxEvalTerm>>& InEvaluationTerms,
        const TArray<TInstancedStruct<FConnectItMinMaxOrderTerm>>& InOrderingTerms);

    // Holds pointers into its own term arrays -- never copied or moved (the
    // strategy keeps it behind a TSharedRef).
    FConnectItMinMaxRules(const FConnectItMinMaxRules&) = delete;
    FConnectItMinMaxRules& operator=(const FConnectItMinMaxRules&) = delete;

    FState MakeRoot(const FConnectItBoardState& Board, int32 SideToMove) const;

    static int32 Opponent(int32 Side) { return 1 - Side; }

    // --- For terms ---

    const FConnectItMinMaxGeometry& GetGeometry() const { return Geometry; }
    int32 GetConnectLength() const { return ConnectLength; }
    float GetWinThreshold() const { return WinThreshold; }

    // --- MinMax::c_game ---

    void GenerateMoves(const FState& State, TArray<FMove>& OutMoves) const;
    FState ApplyMove(const FState& State, const FMove& Move) const;
    bool IsTerminal(const FState& State) const;
    int32 Evaluate(const FState& State, int32 Ply) const;
    int32 OrderScore(const FState& State, const FMove& Move) const;

private:

    int32 ConnectLength = 4;
    float WinThreshold = 0.f;
    FConnectItMinMaxGeometry Geometry;

    // Owned copies of the strategy's terms, and pointers to the usable ones
    // resolved once in the constructor, so the search never touches
    // reflection.
    TArray<TInstancedStruct<FConnectItMinMaxEvalTerm>> EvaluationTerms;
    TArray<TInstancedStruct<FConnectItMinMaxOrderTerm>> OrderingTerms;
    TArray<const FConnectItMinMaxEvalTerm*> ActiveEvaluationTerms;
    TArray<const FConnectItMinMaxOrderTerm*> ActiveOrderingTerms;
};

using FConnectItMinMaxSearch = GameIntelligence::Search::MinMax::TAlphaBeta<FConnectItMinMaxRules>;
