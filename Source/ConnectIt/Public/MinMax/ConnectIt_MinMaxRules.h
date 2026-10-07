// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "GridMechanicsBaseStructs.h"
#include "Misc/TVariant.h"
#include "MinMax/ConnectIt_MinMaxWeights.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Board/Operations/ConnectIt_BoardOperations.h"
#include "GameplayTagContainer.h"
#include "Search/MinMax/GI_MinMaxAlphaBeta.h"


// A candidate move in the MinMax search: one of the board operations the
// search can model. Today that is placing a piece only -- the search assumes
// every move ends the turn and has no use limit. This list is the one place
// to extend when the AI learns another kind of move (the task "Explore
// letting MinMax use moves other than PlacePiece"). A TVariant rather than a
// pointer or FInstancedStruct so a move is a fixed-size value: the search
// creates hundreds of thousands per second.
using FConnectItMinMaxMove = TVariant<FConnectItBoardOperation_PlacePiece>;

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

    // Position -> TileDataArray index (INDEX_NONE if there is no tile there),
    // without searching: a table over the board's bounding box. Moves name
    // grid positions; move ordering, which runs for every candidate move,
    // needs the index.
    int32 TileIndexAt(const FGridPosition& Position) const
    {
        const int32 Column = Position.X - MinX;
        const int32 Row = Position.Y - MinY;
        return Column >= 0 && Column < Width && Row >= 0 && Row < Height
            ? TileIndexGrid[Row * Width + Column]
            : INDEX_NONE;
    }

    static FConnectItMinMaxGeometry Build(const FConnectItBoardState& Board, int32 ConnectLength);

private:

    int32 MinX = 0;
    int32 MinY = 0;
    int32 Width = 0;
    int32 Height = 0;
    TArray<int32> TileIndexGrid;
};

// A ConnectIt match -- two factions, one move per turn -- as the MinMax search
// (GameIntelligence::Search::MinMax::TAlphaBeta) sees it.
//
// Not something you configure or subclass: UConnectIt_AIStrategy_MinMax
// builds one per decision, on the game thread, from the live board and its
// own editor data, then the search only reads it.
//
// It knows no move and no rule of its own:
//   * a move IS a board operation (FConnectItBoardOperation) -- the same
//     struct a player's action sends -- and applying a move is that
//     operation's Apply. Which kinds the search can use is the list in
//     FConnectItMinMaxMove; each side gets the ones it was given request
//     types for;
//   * what follows from a move (scoring) is the rule set's
//     ResolveBoardChange, a separate step after the operation;
//   * when the game is over and who won is the win condition.
// All the same code the server's Mediator runs, so the search can't disagree
// with the game. What the AI VALUES in an unfinished position, and which
// moves it tries first, is the judgement code at the bottom of this class,
// scaled by the strategy's weights (see ConnectIt_MinMaxWeights.h).
//
// Still fixed here: every move ends the turn (see ApplyMove) and there are no
// use limits. That is true of placing a piece; it is why Place Piece is the
// only operation in FConnectItMinMaxMove for now.
//
// Thread safety: the search runs on a background task; rule structs,
// operations and weights are plain data with no UObject references, which is
// what makes this legal.
class CONNECTIT_API FConnectItMinMaxRules final
{
public:

    struct FState
    {
        FConnectItBoardState Board;

        // Faction slot (0 or 1) about to move. EvaluateState scores for this side.
        int32 SideToMove = 0;
    };

    // A move is a board operation (one of the kinds the search can model)
    using FMove = FConnectItMinMaxMove;

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
    // passed to MakeRoot. InRules (the match's rule set) and the weights are
    // copied.
    // Side0RequestTypes / Side1RequestTypes: the request types faction slot 0
    // and 1 may send -- each side moves with the operations of those types
    // (types the search can't model are ignored).
    FConnectItMinMaxRules(
        const FConnectItBoardState& Board,
        const FConnectItRuleSet& InRules,
        const FConnectItMinMaxEvaluationWeights& InEvaluationWeights,
        const FConnectItMinMaxOrderingWeights& InOrderingWeights,
        const FGameplayTagContainer& Side0RequestTypes,
        const FGameplayTagContainer& Side1RequestTypes);

    // Holds pointers into its own rule set -- never copied or moved (the
    // strategy keeps it behind a TSharedRef).
    FConnectItMinMaxRules(const FConnectItMinMaxRules&) = delete;
    FConnectItMinMaxRules& operator=(const FConnectItMinMaxRules&) = delete;

    FState MakeRoot(const FConnectItBoardState& Board, int32 SideToMove) const;

    static int32 Opponent(int32 Side) { return 1 - Side; }

    const FConnectItMinMaxGeometry& GetGeometry() const { return Geometry; }
    const FConnectItRuleSet& GetRuleSet() const { return Rules; }

    // --- MinMax::c_game ---

    // Every operation the side to move could make
    void GenerateMoves(const FState& State, TArray<FMove>& OutMoves) const;

    // Three separate steps: the operation changes the board; the rule set
    // resolves the change (scoring); the turn passes to the other side.
    FState ApplyMove(const FState& State, const FMove& Move) const;

    // Someone has won, per the match's win condition. A position with no
    // legal moves and no winner (e.g. a full board) is deliberately NOT
    // terminal -- it's scored by EvaluateState like any other position. What a
    // full board should mean is an open design question (draw online? usually
    // a loss in adventure? the goal on some levels?) -- see the vault's
    // ConnectIt/_questions/full-board-outcome.md.
    bool IsTerminalState(const FState& State) const;

    // WinValue - Ply / -(WinValue - Ply) / 0 if (somehow) nobody won
    int32 EvaluateTerminalState(const FState& State, int32 Ply) const;

    // How good an unfinished position is for the side to move: the
    // evaluation weights x their factors, kept clear of the win band.
    int32 EvaluateState(const FState& State) const;

    // How promising Move looks before searching it (higher = tried sooner).
    // Each kind of move decides here which tiles matter to it.
    int32 EvaluateMove(const FState& State, const FMove& Move) const;

private:

    // Declaration order matters: the pointers and geometry are initialised
    // from Rules.
    FConnectItRuleSet Rules;
    const FConnectItWinCondition* WinCondition = nullptr;
    FConnectItMinMaxGeometry Geometry;

    // Which kinds of operation each side may make, resolved once from its
    // request types
    bool bSideCanPlace[2] = { false, false };

    FConnectItMinMaxEvaluationWeights EvaluationWeights;
    FConnectItMinMaxOrderingWeights OrderingWeights;

    // --- The factors the weights scale ---
    // Plain functions of a board and exactly what else each one needs.

    // Side's real score minus the opponent's
    static float ScoreDifference(const FConnectItBoardState& Board, int32 Side);

    // Side's own lines in the making: for every connect-length window
    // holding only Side's pieces (and empty, active tiles), pieces² x the
    // window's total multiplier
    float LinePotential(const FConnectItBoardState& Board, int32 Side) const;

    // The score multiplier of a tile
    static float TileMultiplierAt(const FConnectItBoardState& Board, int32 TileIndex);

    // How many of a tile's neighbours hold a piece (either faction)
    int32 CountAdjacentPieces(const FConnectItBoardState& Board, int32 TileIndex) const;
};

using FConnectItMinMaxSearch = GameIntelligence::Search::MinMax::TAlphaBeta<FConnectItMinMaxRules>;
