// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "MinMax/ConnectIt_MinMaxRules.h"
#include "AI/ConnectIt_AIStrategy_MinMax.h"
#include "ConnectIt_MinMaxTestTerms.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ConnectItMinMaxTests
{
    using namespace GameIntelligence::Search::MinMax;

    constexpr EAutomationTestFlags TestFlags =
        EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

    // Empty Size x Size board, two factions, all tiles active, multiplier 1 --
    // the same initial tile data UConnectIt_BoardStateComponent::InitialiseBoardState
    // builds when no designer data is set.
    FConnectItBoardState MakeBoard(int32 Size)
    {
        FConnectItBoardState Board;
        Board.ScoreBoard = { 0.f, 0.f };
        for (int32 X = 0; X < Size; X++)
        {
            for (int32 Y = 0; Y < Size; Y++)
            {
                FConnectItTileData Tile;
                Tile.SetFactionPiece(-1);
                Board.SetTileData(FGridPosition(X, Y), Tile);
            }
        }
        return Board;
    }

    void Place(FConnectItBoardState& Board, int32 X, int32 Y, int32 Faction)
    {
        Board.GetTileDataMutable(FGridPosition(X, Y))->SetFactionPiece(Faction);
    }

    FParams Depth(int32 MaxDepth)
    {
        FParams Params;
        Params.MaxDepth = MaxDepth;
        return Params;
    }

    // The MinMax strategy's own defaults, so tests exercise what ships
    const TArray<TInstancedStruct<FConnectItMinMaxEvalTerm>>& DefaultEvaluationTerms()
    {
        return GetDefault<UConnectIt_AIStrategy_MinMax>()->EvaluationTerms;
    }

    const TArray<TInstancedStruct<FConnectItMinMaxOrderTerm>>& DefaultOrderingTerms()
    {
        return GetDefault<UConnectIt_AIStrategy_MinMax>()->OrderingTerms;
    }

    bool IsPosition(const FConnectItMinMaxRules::FMove& Move, int32 X, int32 Y)
    {
        return Move.Position.X == X && Move.Position.Y == Y;
    }
}

using namespace ConnectItMinMaxTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMinMaxTakesWinTest,
    "ConnectIt.AI.MinMax.TakesImmediateWin", TestFlags)

bool FConnectItMinMaxTakesWinTest::RunTest(const FString& Parameters)
{
    // Faction 0 has three in a row along Y at X=3 ending at the board edge, so
    // (3,3) is the only completing tile; four multiplier-1 tiles = 4 points,
    // which is the win threshold.
    FConnectItBoardState Board = MakeBoard(7);
    Place(Board, 3, 0, 0);
    Place(Board, 3, 1, 0);
    Place(Board, 3, 2, 0);
    Place(Board, 0, 6, 1);
    Place(Board, 6, 6, 1);

    const FConnectItMinMaxRules Rules(Board, 4, 4.f, DefaultEvaluationTerms(), DefaultOrderingTerms());
    const auto Root = Rules.MakeRoot(Board, 0);
    const auto Result = FConnectItMinMaxSearch::Run(Rules, Root, Depth(3));

    if (!TestTrue(TEXT("search returned moves"), Result.RootScores.Num() > 0)) return false;
    TestTrue(TEXT("best move completes the line at (3,3)"), IsPosition(Result.RootScores[0].Move, 3, 3));
    TestTrue(TEXT("best move is scored as a win"),
        Result.RootScores[0].Score > FConnectItMinMaxRules::WinValue / 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMinMaxBlocksLossTest,
    "ConnectIt.AI.MinMax.BlocksOpponentWin", TestFlags)

bool FConnectItMinMaxBlocksLossTest::RunTest(const FString& Parameters)
{
    // Faction 1 threatens to complete (0,1)-(3,1) along X; (3,1) is the only
    // completing tile because the line starts at the board edge. Faction 0
    // (the searcher) has no threat of its own, so it must block.
    FConnectItBoardState Board = MakeBoard(7);
    Place(Board, 0, 1, 1);
    Place(Board, 1, 1, 1);
    Place(Board, 2, 1, 1);
    Place(Board, 6, 6, 0);

    const FConnectItMinMaxRules Rules(Board, 4, 4.f, DefaultEvaluationTerms(), DefaultOrderingTerms());
    const auto Root = Rules.MakeRoot(Board, 0);
    const auto Result = FConnectItMinMaxSearch::Run(Rules, Root, Depth(2));

    if (!TestTrue(TEXT("search returned moves"), Result.RootScores.Num() > 0)) return false;
    TestTrue(TEXT("best move blocks at (3,1)"), IsPosition(Result.RootScores[0].Move, 3, 1));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMinMaxLegalMovesTest,
    "ConnectIt.AI.MinMax.OnlyLegalMoves", TestFlags)

bool FConnectItMinMaxLegalMovesTest::RunTest(const FString& Parameters)
{
    FConnectItBoardState Board = MakeBoard(5);
    Board.GetTileDataMutable(FGridPosition(2, 2))->bIsActive = false;
    Board.GetTileDataMutable(FGridPosition(0, 4))->bIsActive = false;
    Place(Board, 1, 1, 0);
    Place(Board, 3, 3, 1);

    // A non-faction blocker: occupied, but nobody's piece
    Board.GetTileDataMutable(FGridPosition(4, 0))->bIsOccupied = true;

    const FConnectItMinMaxRules Rules(Board, 4, 100.f, DefaultEvaluationTerms(), DefaultOrderingTerms());
    const auto Root = Rules.MakeRoot(Board, 1);
    const auto Result = FConnectItMinMaxSearch::Run(Rules, Root, Depth(2));

    TestEqual(TEXT("one root move per active, unoccupied tile"), Result.RootScores.Num(), 25 - 5);
    for (const auto& Scored : Result.RootScores)
    {
        const FConnectItTileData* Tile = Board.GetTileData(Scored.Move.Position);
        TestTrue(FString::Printf(TEXT("(%d,%d) is placeable"),
                Scored.Move.Position.X, Scored.Move.Position.Y),
            Tile && Tile->bIsActive && !Tile->bIsOccupied);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMinMaxRootSideTest,
    "ConnectIt.AI.MinMax.RootMovesAreOwnFaction", TestFlags)

bool FConnectItMinMaxRootSideTest::RunTest(const FString& Parameters)
{
    // Regression for the 09-24 off-by-one: the root's moves must place the
    // searcher's own piece, and the turn then passes to the other faction.
    const FConnectItBoardState Board = MakeBoard(5);
    const FConnectItMinMaxRules Rules(Board, 4, 100.f, DefaultEvaluationTerms(), DefaultOrderingTerms());
    const auto Root = Rules.MakeRoot(Board, 1);

    TArray<FConnectItMinMaxRules::FMove> Moves;
    Rules.GenerateMoves(Root, Moves);
    if (!TestTrue(TEXT("root has moves"), Moves.Num() > 0)) return false;

    const auto Child = Rules.ApplyMove(Root, Moves[0]);
    TestEqual(TEXT("placed piece belongs to the root side"),
        Child.Board.GetTileDataAt(Moves[0].TileIndex).FactionPiece, 1);
    TestEqual(TEXT("side to move flips"), Child.SideToMove, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMinMaxCancelTest,
    "ConnectIt.AI.MinMax.RespectsCancel", TestFlags)

bool FConnectItMinMaxCancelTest::RunTest(const FString& Parameters)
{
    const FConnectItMinMaxRules Rules(MakeBoard(7), 4, 100.f, DefaultEvaluationTerms(), DefaultOrderingTerms());
    const auto Root = Rules.MakeRoot(MakeBoard(7), 0);

    std::atomic<bool> Cancelled(true);
    FParams Params = Depth(4);
    Params.CancelFlag = &Cancelled;

    const auto Result = FConnectItMinMaxSearch::Run(Rules, Root, Params);
    TestTrue(TEXT("result is marked cancelled"), Result.bCancelled);
    TestEqual(TEXT("no root scores from a cancelled search"), Result.RootScores.Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMinMaxThroughputTest,
    "ConnectIt.AI.MinMax.Throughput", TestFlags)

bool FConnectItMinMaxThroughputTest::RunTest(const FString& Parameters)
{
    // Not a pass/fail on speed -- reports how deep the default time budget
    // gets on a 7x7 mid-game board, to sanity-check UConnectIt_AIStrategy_MinMax's
    // defaults (MaxDepth 4, 1.5 s).
    FConnectItBoardState Board = MakeBoard(7);
    Place(Board, 3, 3, 0);
    Place(Board, 2, 3, 1);
    Place(Board, 3, 4, 0);
    Place(Board, 4, 2, 1);
    Place(Board, 2, 2, 0);
    Place(Board, 4, 4, 1);

    const FConnectItMinMaxRules Rules(Board, 4, 100.f, DefaultEvaluationTerms(), DefaultOrderingTerms());
    const auto Root = Rules.MakeRoot(Board, 0);

    FParams Params;
    Params.MaxDepth = 8;
    Params.TimeBudgetSeconds = 1.5;

    const auto Result = FConnectItMinMaxSearch::Run(Rules, Root, Params);

    AddInfo(FString::Printf(TEXT("depth %d%s, %lld nodes in %.0f ms (%.0f nodes/s)"),
        Result.DepthReached, Result.bOutOfTime ? TEXT(" (out of time)") : TEXT(""),
        Result.NodesVisited, Result.ElapsedSeconds * 1000.0,
        Result.ElapsedSeconds > 0.0 ? Result.NodesVisited / Result.ElapsedSeconds : 0.0));

    TestTrue(TEXT("at least depth 1 completed"), Result.DepthReached >= 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMinMaxTermsDriveChoiceTest,
    "ConnectIt.AI.MinMax.EvaluationTermsDriveChoice", TestFlags)

bool FConnectItMinMaxTermsDriveChoiceTest::RunTest(const FString& Parameters)
{
    // Same search, same rules class -- only the evaluation term list differs,
    // which is exactly what a designer changes on the MinMax strategy.
    const FConnectItBoardState Board = MakeBoard(5);

    TArray<TInstancedStruct<FConnectItMinMaxEvalTerm>> CornerOnly;
    CornerOnly.Add(TInstancedStruct<FConnectItMinMaxEvalTerm>::Make<FConnectItMinMaxEvalTerm_TestCorner>());

    const FConnectItMinMaxRules Defaults(Board, 4, 100.f, DefaultEvaluationTerms(), DefaultOrderingTerms());
    const FConnectItMinMaxRules Corner(Board, 4, 100.f, CornerOnly, DefaultOrderingTerms());

    const auto DefaultResult = FConnectItMinMaxSearch::Run(Defaults, Defaults.MakeRoot(Board, 0), Depth(1));
    const auto CornerResult = FConnectItMinMaxSearch::Run(Corner, Corner.MakeRoot(Board, 0), Depth(1));

    if (!TestTrue(TEXT("both searches returned moves"),
        DefaultResult.RootScores.Num() > 0 && CornerResult.RootScores.Num() > 0)) return false;

    TestTrue(TEXT("with only the corner term, the AI takes the corner"),
        IsPosition(CornerResult.RootScores[0].Move, 0, 0));
    TestFalse(TEXT("with the default terms, it doesn't"),
        IsPosition(DefaultResult.RootScores[0].Move, 0, 0));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
