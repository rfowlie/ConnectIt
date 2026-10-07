// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "GridMechanicsBaseStructs.h"
#include "StructUtils/InstancedStruct.h"
#include "MinMax/ConnectIt_MinMaxTerms.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Search/MinMax/GI_MinMaxAlphaBeta.h"


// Board geometry for one decision: depends only on tile positions and the
// connect length, never on pieces, so it's built once (game thread) and read
// by every node of the search.
struct FConnectItMinMaxGeometry
{
    // Every run of ConnectLength existing tiles along one of the scoring
    // axes (FConnectItScoringRule_Lines::GetScoringDirections), as indices
    // into FConnectItBoardState::TileDataArray. Empty unless the match scores
    // by lines (ConnectLength <= 0 builds none).
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
// own editor data, then the search only reads it.
//
// It holds a copy of the match's FConnectItRuleSet and plays by it: which
// tiles can be played is the real placement rule, what a placement scores is
// the real scoring rule, when the game is over and who won is the real win
// condition -- the same rule code the server's Mediator runs, so the search
// can't disagree with the game. What the AI VALUES in an unfinished position
// comes from the strategy's evaluation and ordering terms (see
// ConnectIt_MinMaxTerms.h).
//
// Still fixed here, for now: the only move is "place one piece", and turns
// alternate one placement each (see the design note on moves as shared,
// thread-safe types: ConnectIt/design/rules-as-structs-and-shared-simulation).
//
// Thread safety: the search runs on a background task; rule structs and terms
// are plain data with no UObject references, which is what makes this legal.
class CONNECTIT_API FConnectItMinMaxRules final
{
public:

    struct FState
    {
        FConnectItBoardState Board;

        // Faction slot (0 or 1) about to place. EvaluateState scores for this side.
        int32 SideToMove = 0;
    };

    struct FMove
    {
        int32 TileIndex = INDEX_NONE;
        FGridPosition Position;
    };

    // Win/loss score, adjusted by Ply (moves from the search root -- see
    // TAlphaBeta::Negamax). A win Ply moves away is worth WinValue - Ply:
    //   AI wins on its next move     -> found at Ply 1 -> WinValue - 1
    //   AI wins after AI/human/AI    -> found at Ply 3 -> WinValue - 3
    // so the AI takes the quickest win instead of dawdling. A loss is
    // -(WinValue - Ply): losing in 4 moves (-(WinValue - 4)) scores higher
    // than losing in 2 (-(WinValue - 2)), so when a loss can't be avoided
    // the AI delays it, giving the opponent more chances to go wrong.
    static constexpr int32 WinValue = 10000000;

    // Board is only used for its geometry -- the position to search from is
    // passed to MakeRoot. InRules (the match's rule set) and the term arrays
    // are copied; invalid term entries and zero-weight terms are skipped.
    FConnectItMinMaxRules(
        const FConnectItBoardState& Board,
        const FConnectItRuleSet& InRules,
        const TArray<TInstancedStruct<FConnectItMinMaxEvalTerm>>& InEvaluationTerms,
        const TArray<TInstancedStruct<FConnectItMinMaxOrderTerm>>& InOrderingTerms);

    // Holds pointers into its own rule set and term arrays -- never copied or
    // moved (the strategy keeps it behind a TSharedRef).
    FConnectItMinMaxRules(const FConnectItMinMaxRules&) = delete;
    FConnectItMinMaxRules& operator=(const FConnectItMinMaxRules&) = delete;

    FState MakeRoot(const FConnectItBoardState& Board, int32 SideToMove) const;

    static int32 Opponent(int32 Side) { return 1 - Side; }

    // --- For terms ---

    const FConnectItMinMaxGeometry& GetGeometry() const { return Geometry; }
    const FConnectItRuleSet& GetRuleSet() const { return Rules; }

    // --- MinMax::c_game ---

    void GenerateMoves(const FState& State, TArray<FMove>& OutMoves) const;
    FState ApplyMove(const FState& State, const FMove& Move) const;

    // Someone has won, per the match's win condition. A position with no
    // legal moves and no winner (e.g. a full board) is deliberately NOT
    // terminal -- it's scored by the terms like any other position. What a
    // full board should mean is an open design question (draw online? usually
    // a loss in adventure? the goal on some levels?) -- see the vault's
    // ConnectIt/_questions/full-board-outcome.md.
    bool IsTerminalState(const FState& State) const;

    // WinValue - Ply / -(WinValue - Ply) / 0 if (somehow) nobody won
    int32 EvaluateTerminalState(const FState& State, int32 Ply) const;

    // Unfinished positions only: sum of Weight x term, then the clamp
    int32 EvaluateState(const FState& State) const;
    int32 EvaluateMove(const FState& State, const FMove& Move) const;

private:

    // Declaration order matters: the pointers and geometry are initialised
    // from Rules.
    FConnectItRuleSet Rules;
    const FConnectItScoringRule* ScoringRule = nullptr;
    const FConnectItWinCondition* WinCondition = nullptr;
    const FConnectItTilePlaceableRule* TilePlaceableRule = nullptr;
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
