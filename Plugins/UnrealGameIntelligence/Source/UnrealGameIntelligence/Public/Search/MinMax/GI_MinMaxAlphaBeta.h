// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Algo/Sort.h"
#include "HAL/PlatformTime.h"
#include <atomic>
#include "Search/GI_SearchAsync.h"


// Generic two-player, zero-sum game-tree search: negamax with alpha-beta
// pruning, move ordering and iterative deepening.
//
// Unlike the older MinMax/ templates (which build and store a whole tree,
// then walk it), this search generates each node's children on demand,
// depth first, and keeps nothing beyond the current path -- so alpha-beta
// cutoffs skip generation as well as evaluation, and memory stays at one
// state per ply.
//
// A game plugs in through a rules object of type TGame -- see c_game. The
// search only ever calls it through a const reference, from whichever thread
// runs the search (normally a background task, see GI_SearchAsync.h), so the
// object must be safe to read off the game thread and must not change while
// a search is running.
namespace GameIntelligence::Search::MinMax
{
    // TGame::FState     -- a complete game position, including whose move it is
    // TGame::FMove      -- one move from a position
    // GenerateMoves     -- every legal move from State, appended to OutMoves
    // ApplyMove         -- the position after Move is played from State (and
    //                      the other side is to move)
    // IsTerminalState   -- the game is over in State (someone has won, or
    //                      whatever else ends this game)
    // EvaluateTerminalState
    //                   -- score of a FINISHED State from the side to move's
    //                      point of view (win / loss / draw). Ply is how many
    //                      moves State is from the search root (see Negamax)
    //                      -- use it to make a win worth less the further
    //                      away it is, so the search prefers faster wins and
    //                      slower losses
    // EvaluateState     -- score of an UNFINISHED State from the side to move's
    //                      point of view (negamax): the game's judgement of
    //                      who is better placed. Never called for a finished
    //                      State
    // EvaluateMove      -- cheap "how promising is Move" guess, used only to
    //                      order moves: higher is tried first, which makes
    //                      alpha-beta prune more
    //
    // All are const member functions, so a rules object can carry per-search
    // configuration (weights, precomputed geometry) and a variant can derive
    // from it and override only what differs.
    template<typename TGame>
    concept c_game = requires(
        const TGame& Game,
        const typename TGame::FState& State,
        const typename TGame::FMove& Move,
        TArray<typename TGame::FMove>& OutMoves,
        int32 Ply)
    {
        { Game.GenerateMoves(State, OutMoves) } -> std::same_as<void>;
        { Game.ApplyMove(State, Move) } -> std::same_as<typename TGame::FState>;
        { Game.IsTerminalState(State) } -> std::same_as<bool>;
        { Game.EvaluateTerminalState(State, Ply) } -> std::convertible_to<int32>;
        { Game.EvaluateState(State) } -> std::convertible_to<int32>;
        { Game.EvaluateMove(State, Move) } -> std::convertible_to<int32>;
    };

    // Scores are kept well inside int32 so negating never overflows.
    inline constexpr int32 ScoreInfinity = 1'000'000'000;

    struct FParams
    {
        // Deepest iteration to run (plies from the root, >= 1)
        int32 MaxDepth = 1;

        // Wall-clock budget; 0 = no limit. Depth 1 always completes, so there
        // is always a result. A deeper iteration cut off by the budget is
        // discarded -- the result is the last iteration that finished.
        double TimeBudgetSeconds = 0.0;

        // Optional external stop request, checked before each depth and
        // periodically during it. Unlike the time budget it also stops depth
        // 1; a cancelled result has bCancelled set and no RootScores.
        const std::atomic<bool>* CancelFlag = nullptr;
    };

    template<typename TMove>
    struct TScoredMove
    {
        TMove Move;
        int32 Score = 0;
    };

    template<typename TMove>
    struct TResult
    {
        // Every root move with its exact score at DepthReached, the best first.
        // Root moves are searched with a full window (no pruning between
        // root siblings), so these are true scores, not bounds -- callers can
        // pick a deliberately weaker move from them.
        TArray<TScoredMove<TMove>> RootScores;

        int32 DepthReached = 0;
        int64 NodesVisited = 0;
        double ElapsedSeconds = 0.0;
        bool bCancelled = false;
        bool bOutOfTime = false;
    };

    template<typename TGame> requires c_game<TGame>
    class TAlphaBeta
    {
    public:
        using FState = typename TGame::FState;
        using FMove = typename TGame::FMove;
        using FResult = TResult<FMove>;

        static FResult Run(const TGame& Game, const FState& Root, const FParams& Params)
        {
            FContext Context;
            Context.Params = Params;
            Context.StartTime = FPlatformTime::Seconds();

            FResult Result;

            // early out
            if (Game.IsTerminalState(Root)) return Result;

            TArray<FMove> RootMoves;
            Game.GenerateMoves(Root, RootMoves);
            if (RootMoves.IsEmpty()) return Result;

            // initial estimation of move effectiveness
            OrderMoves(Game, Root, RootMoves);

            // ensure at least a depth of 1
            const int32 MaxDepth = FMath::Max(1, Params.MaxDepth);
            for (int32 Depth = 1; Depth <= MaxDepth; Depth++)
            {
                if (Context.IsCancelled())
                {
                    Context.bAborted = true;
                    break;
                }

                // The time budget only applies once a full iteration exists
                Context.bEnforceTimeBudget = Depth > 1;

                TArray<TScoredMove<FMove>> DepthScores;
                DepthScores.Reserve(RootMoves.Num());

                for (const FMove& Move : RootMoves)
                {
                    // Child is one move from the root, hence Ply 1
                    const FState Child = Game.ApplyMove(Root, Move);
                    const int32 Score = -Negamax(
                        Game, Child, Depth - 1, -ScoreInfinity, ScoreInfinity, 1, Context);

                    if (Context.bAborted) break;
                    DepthScores.Add({ Move, Score });
                }

                if (Context.bAborted) break;

                // Stable sort: equal scores keep the previous iteration's order
                Algo::StableSort(DepthScores,
                    [](const TScoredMove<FMove>& A, const TScoredMove<FMove>& B)
                    {
                        return A.Score > B.Score;
                    });

                Result.RootScores = DepthScores;
                Result.DepthReached = Depth;

                // Best-first next iteration: the best move so far is searched
                // first, which tightens the inner windows soonest
                RootMoves.Reset();
                for (const TScoredMove<FMove>& Scored : DepthScores)
                {
                    RootMoves.Add(Scored.Move);
                }

                if (Context.IsOutOfTime()) break;
            }

            Result.NodesVisited = Context.Nodes;
            Result.ElapsedSeconds = FPlatformTime::Seconds() - Context.StartTime;
            Result.bCancelled = Context.IsCancelled();
            Result.bOutOfTime = Context.bAborted && !Result.bCancelled;

            // A cancelled search's caller has moved on -- hand back nothing
            // it could mistakenly act on
            if (Result.bCancelled)
            {
                Result.RootScores.Reset();
                Result.DepthReached = 0;
            }

            return Result;
        }

    private:

        struct FContext
        {
            FParams Params;
            double StartTime = 0.0;
            int64 Nodes = 0;
            bool bEnforceTimeBudget = false;
            bool bAborted = false;

            bool IsCancelled() const
            {
                return Params.CancelFlag && Params.CancelFlag->load(std::memory_order_relaxed);
            }

            bool IsOutOfTime() const
            {
                return Params.TimeBudgetSeconds > 0.0
                    && FPlatformTime::Seconds() - StartTime >= Params.TimeBudgetSeconds;
            }

            // Polled every 256 nodes -- cheap enough not to show up, frequent
            // enough to stop within a few milliseconds
            void Poll()
            {
                if ((Nodes & 255) != 0) return;
                if (IsCancelled() || (bEnforceTimeBudget && IsOutOfTime()))
                {
                    bAborted = true;
                }
            }
        };

        static void OrderMoves(const TGame& Game, const FState& State, TArray<FMove>& Moves)
        {
            TArray<TPair<int32, FMove>> Keyed;
            Keyed.Reserve(Moves.Num());
            for (const FMove& Move : Moves)
            {
                Keyed.Emplace(Game.EvaluateMove(State, Move), Move);
            }

            Algo::StableSort(Keyed, [](const TPair<int32, FMove>& A, const TPair<int32, FMove>& B)
            {
                return A.Key > B.Key;
            });

            for (int32 Index = 0; Index < Keyed.Num(); Index++)
            {
                Moves[Index] = Keyed[Index].Value;
            }
        }

        // Score of State for the side to move. Returns 0 once aborted -- the
        // caller discards the whole iteration in that case.
        //
        // Depth and Ply count opposite ways:
        //   Depth -- moves still to search below this node (counts DOWN to 0)
        //   Ply   -- moves already played from the search root to reach this
        //            node (counts UP: the root's children are Ply 1, theirs
        //            Ply 2, ...). At any node, Depth + Ply = this iteration's
        //            depth.
        // Ply is passed down rather than derived from Depth because iterative
        // deepening runs the search at depth 1, 2, 3...: the same Depth value
        // means a different distance from the root in each iteration.
        //
        // Ply is only for EvaluateTerminalState, so a game can rank wins and losses by
        // how soon they happen (e.g. return WinValue - Ply for a win). Without
        // it a win next move and a win in three moves score the same, so the
        // search may dawdle toward a win it could take now; and every loss
        // looks equally bad, so when losing is unavoidable it doesn't delay
        // it. With it: nearest win first, and the farthest loss is preferred
        // -- more moves for the opponent to go wrong. (Chess engines do the
        // same for checkmate distance.)
        static int32 Negamax(
            const TGame& Game, const FState& State, int32 Depth, int32 Alpha, int32 Beta,
            int32 Ply, FContext& Context)
        {
            ++Context.Nodes;
            Context.Poll();
            if (Context.bAborted) return 0;

            // A finished game is scored as a result (win/loss/draw), never by
            // the position heuristic
            if (Game.IsTerminalState(State))
            {
                return Game.EvaluateTerminalState(State, Ply);
            }

            if (Depth <= 0)
            {
                return Game.EvaluateState(State);
            }

            // No legal moves but not finished: the game decided this isn't an
            // end state, so judge it like any other unfinished position
            TArray<FMove> Moves;
            Game.GenerateMoves(State, Moves);
            if (Moves.IsEmpty())
            {
                return Game.EvaluateState(State);
            }

            OrderMoves(Game, State, Moves);

            int32 Best = -ScoreInfinity;
            for (const FMove& Move : Moves)
            {
                const FState Child = Game.ApplyMove(State, Move);
                const int32 Score = -Negamax(Game, Child, Depth - 1, -Beta, -Alpha, Ply + 1, Context);
                if (Context.bAborted) return 0;

                Best = FMath::Max(Best, Score);
                Alpha = FMath::Max(Alpha, Score);
                if (Alpha >= Beta) break; // the opponent will never allow this line
            }

            return Best;
        }
    };

    // Runs TAlphaBeta<TGame> on a background task (see Search::LaunchAsync).
    // The task holds Game and CancelFlag by shared reference and Root by
    // value, so all three stay alive for the whole search whatever the
    // caller does next. OnComplete runs on the game thread, cancelled or not
    // (check Result.bCancelled).
    template<typename TGame> requires c_game<TGame>
    void LaunchAlphaBetaAsync(
        TSharedRef<const TGame, ESPMode::ThreadSafe> Game,
        typename TGame::FState Root,
        FParams Params,
        FCancelFlag CancelFlag,
        TUniqueFunction<void(TResult<typename TGame::FMove>&&)> OnComplete)
    {
        using FResultType = TResult<typename TGame::FMove>;

        LaunchAsync<FResultType>(
            [Game, Root = MoveTemp(Root), Params, CancelFlag]() mutable -> FResultType
            {
                Params.CancelFlag = &CancelFlag.Get();
                return TAlphaBeta<TGame>::Run(*Game, Root, Params);
            },
            MoveTemp(OnComplete));
    }
}
