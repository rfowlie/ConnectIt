// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "MinMax/ConnectIt_MinMaxRules.h"
#include "AI/ConnectIt_AIStrategy_MinMax.h"
#include "ConnectIt_MinMaxTestTerms.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Board/Operations/ConnectIt_BoardOperations.h"
#include "ConnectIt_GameplayTags.h"

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
    const FConnectItMinMaxEvaluationWeights& DefaultEvaluationWeights()
    {
        return GetDefault<UConnectIt_AIStrategy_MinMax>()->EvaluationWeights;
    }

    const FConnectItMinMaxOrderingWeights& DefaultOrderingWeights()
    {
        return GetDefault<UConnectIt_AIStrategy_MinMax>()->OrderingWeights;
    }

    // The classic rule set (lines of 4) won at Threshold
    FConnectItRuleSet RulesWonAt(float Threshold)
    {
        FConnectItRuleSet Rules;
        Rules.SetTargetScore(Threshold);
        return Rules;
    }

    // Both sides can place pieces, and nothing else -- what the MinMax
    // strategy hands the search today
    FGameplayTagContainer PlaceOnly()
    {
        return FGameplayTagContainer(ConnectIt_Game_PlacePiece);
    }

    // A search move that places a piece at (X, Y)
    bool IsPosition(const FConnectItMinMaxMove& Move, int32 X, int32 Y)
    {
        const FConnectItBoardOperation_PlacePiece* Place = Move.TryGet<FConnectItBoardOperation_PlacePiece>();
        return Place && Place->Position == FGridPosition(X, Y);
    }

    FGridPosition PlacePosition(const FConnectItMinMaxMove& Move)
    {
        return Move.Get<FConnectItBoardOperation_PlacePiece>().Position;
    }

    FConnectItMinMaxMove PlaceMove(int32 Faction, int32 X, int32 Y)
    {
        FConnectItBoardOperation_PlacePiece Operation;
        Operation.Faction = Faction;
        Operation.Position = FGridPosition(X, Y);
        return FConnectItMinMaxMove(TInPlaceType<FConnectItBoardOperation_PlacePiece>(), Operation);
    }

    // Any operation with a single Position field, for Faction at (X, Y)
    template<typename TOperation>
    TOperation MakeAt(int32 Faction, int32 X, int32 Y)
    {
        TOperation Operation;
        Operation.Faction = Faction;
        Operation.Position = FGridPosition(X, Y);
        return Operation;
    }

    const FConnectItTileData& TileAt(const FConnectItBoardState& Board, int32 X, int32 Y)
    {
        return *Board.GetTileData(FGridPosition(X, Y));
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

    const FConnectItMinMaxRules Rules(Board, RulesWonAt(4.f), DefaultEvaluationWeights(), DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());
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

    const FConnectItMinMaxRules Rules(Board, RulesWonAt(4.f), DefaultEvaluationWeights(), DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());
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

    const FConnectItMinMaxRules Rules(Board, RulesWonAt(100.f), DefaultEvaluationWeights(), DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());
    const auto Root = Rules.MakeRoot(Board, 1);
    const auto Result = FConnectItMinMaxSearch::Run(Rules, Root, Depth(2));

    TestEqual(TEXT("one root move per active, unoccupied tile"), Result.RootScores.Num(), 25 - 5);
    for (const auto& Scored : Result.RootScores)
    {
        const FGridPosition Position = PlacePosition(Scored.Move);
        const FConnectItTileData* Tile = Board.GetTileData(Position);
        TestTrue(FString::Printf(TEXT("(%d,%d) is placeable"), Position.X, Position.Y),
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
    const FConnectItMinMaxRules Rules(Board, RulesWonAt(100.f), DefaultEvaluationWeights(), DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());
    const auto Root = Rules.MakeRoot(Board, 1);

    TArray<FConnectItMinMaxRules::FMove> Moves;
    Rules.GenerateMoves(Root, Moves);
    if (!TestTrue(TEXT("root has moves"), Moves.Num() > 0)) return false;

    const auto Child = Rules.ApplyMove(Root, Moves[0]);
    TestEqual(TEXT("placed piece belongs to the root side"),
        Child.Board.GetTileData(PlacePosition(Moves[0]))->FactionPiece, 1);
    TestEqual(TEXT("side to move flips"), Child.SideToMove, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMinMaxCancelTest,
    "ConnectIt.AI.MinMax.RespectsCancel", TestFlags)

bool FConnectItMinMaxCancelTest::RunTest(const FString& Parameters)
{
    const FConnectItMinMaxRules Rules(MakeBoard(7), RulesWonAt(100.f), DefaultEvaluationWeights(), DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());
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

    const FConnectItMinMaxRules Rules(Board, RulesWonAt(100.f), DefaultEvaluationWeights(), DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMinMaxWeightsDriveChoiceTest,
    "ConnectIt.AI.MinMax.EvaluationWeightsDriveChoice", TestFlags)

bool FConnectItMinMaxWeightsDriveChoiceTest::RunTest(const FString& Parameters)
{
    // Same search, same rules -- only the evaluation weights differ, which is
    // exactly what a designer changes on the MinMax strategy. Faction 0 has
    // three in a row: completing it at (3,3) scores real points but clears
    // the line, and with it the line's potential.
    FConnectItBoardState Board = MakeBoard(7);
    Place(Board, 3, 0, 0);
    Place(Board, 3, 1, 0);
    Place(Board, 3, 2, 0);

    FConnectItMinMaxEvaluationWeights PointsOnly;
    PointsOnly.ScoreDifference = 1000.f;
    PointsOnly.LinePotential = 0.f;

    FConnectItMinMaxEvaluationWeights PotentialOnly;
    PotentialOnly.ScoreDifference = 0.f;
    PotentialOnly.LinePotential = 10.f;

    const FConnectItMinMaxRules Points(Board, RulesWonAt(100.f), PointsOnly, DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());
    const FConnectItMinMaxRules Potential(Board, RulesWonAt(100.f), PotentialOnly, DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());

    const auto PointsResult = FConnectItMinMaxSearch::Run(Points, Points.MakeRoot(Board, 0), Depth(1));
    const auto PotentialResult = FConnectItMinMaxSearch::Run(Potential, Potential.MakeRoot(Board, 0), Depth(1));

    if (!TestTrue(TEXT("both searches returned moves"),
        PointsResult.RootScores.Num() > 0 && PotentialResult.RootScores.Num() > 0)) return false;

    TestTrue(TEXT("valuing only real points, the AI completes the line"),
        IsPosition(PointsResult.RootScores[0].Move, 3, 3));
    TestFalse(TEXT("valuing only line potential, it keeps the line instead"),
        IsPosition(PotentialResult.RootScores[0].Move, 3, 3));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMinMaxWinConditionTest,
    "ConnectIt.AI.MinMax.WinConditionDrivesTerminal", TestFlags)

bool FConnectItMinMaxWinConditionTest::RunTest(const FString& Parameters)
{
    // A win condition the search knows nothing about (not score-based): set
    // on the rule set like a designer would, the AI goes for it with no AI
    // changes -- and scores it as a win, not via the evaluation terms.
    const FConnectItBoardState Board = MakeBoard(5);
    FConnectItRuleSet CornerRules;
    CornerRules.WinCondition =
        TInstancedStruct<FConnectItWinCondition>::Make<FConnectItWinCondition_TestCorner>();

    const FConnectItMinMaxRules Rules(Board, CornerRules,
        DefaultEvaluationWeights(), DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());

    const auto Result = FConnectItMinMaxSearch::Run(Rules, Rules.MakeRoot(Board, 0), Depth(2));

    if (!TestTrue(TEXT("search returned moves"), Result.RootScores.Num() > 0)) return false;
    TestTrue(TEXT("the AI takes the winning corner"), IsPosition(Result.RootScores[0].Move, 0, 0));
    TestTrue(TEXT("and scores it as a win"),
        Result.RootScores[0].Score > FConnectItMinMaxRules::WinValue / 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMinMaxSameRulesTest,
    "ConnectIt.AI.MinMax.RuleSetScoresLikeTheGame", TestFlags)

bool FConnectItMinMaxSameRulesTest::RunTest(const FString& Parameters)
{
    // The 4th piece of a line, applied two ways: the way the server's
    // Mediator does it (place, then FConnectItRuleSet::ApplyScoring) and
    // through the search's ApplyMove. Same rule code, so the same board.
    FConnectItBoardState Board = MakeBoard(7);
    Place(Board, 3, 0, 0);
    Place(Board, 3, 1, 0);
    Place(Board, 3, 2, 0);

    const FConnectItRuleSet RuleSet = RulesWonAt(100.f);

    FConnectItBoardState GameBoard = Board;
    Place(GameBoard, 3, 3, 0);
    const float Points = RuleSet.ApplyScoring(GameBoard, FGridPosition(3, 3), 0);

    const FConnectItMinMaxRules Rules(Board, RuleSet, DefaultEvaluationWeights(), DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());
    const auto Root = Rules.MakeRoot(Board, 0);
    const auto Child = Rules.ApplyMove(Root, PlaceMove(0, 3, 3));

    TestEqual(TEXT("the game scored the line (4 tiles x multiplier 1)"), Points, 4.f);
    TestEqual(TEXT("same score"), Child.Board.GetScore(0), GameBoard.GetScore(0));

    bool bSameTiles = Child.Board.NumTiles() == GameBoard.NumTiles();
    for (int32 Index = 0; bSameTiles && Index < GameBoard.NumTiles(); Index++)
    {
        const FConnectItTileData& A = GameBoard.GetTileDataAt(Index);
        const FConnectItTileData& B = Child.Board.GetTileDataAt(Index);
        bSameTiles = A.FactionPiece == B.FactionPiece && A.Multiplier == B.Multiplier
            && A.bIsOccupied == B.bIsOccupied && A.bIsActive == B.bIsActive;
    }
    TestTrue(TEXT("same pieces and multipliers on every tile"), bSameTiles);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItPlaceOperationTest,
    "ConnectIt.AI.MinMax.PlaceOperation", TestFlags)

bool FConnectItPlaceOperationTest::RunTest(const FString& Parameters)
{
    FConnectItBoardState Board = MakeBoard(5);
    Place(Board, 1, 1, 1);
    Board.GetTileDataMutable(FGridPosition(2, 2))->bIsActive = false;

    using FPlace = FConnectItBoardOperation_PlacePiece;

    int32 NumMoves = 0;
    bool bAllOwnFaction = true;
    FPlace::ForEachMove(Board, 0, [&](const FPlace& Operation)
    {
        NumMoves++;
        bAllOwnFaction &= Operation.Faction == 0 && Operation.CanApply(Board);
    });
    TestEqual(TEXT("one move per free, active tile"), NumMoves, 25 - 2);
    TestTrue(TEXT("each is a valid move for the faction asked for"), bAllOwnFaction);

    const FPlace Free = MakeAt<FPlace>(0, 0, 0);
    TestTrue(TEXT("a free tile is valid"), Free.CanApply(Board));
    TestFalse(TEXT("an occupied tile is not"), MakeAt<FPlace>(0, 1, 1).CanApply(Board));
    TestFalse(TEXT("an inactive tile is not"), MakeAt<FPlace>(0, 2, 2).CanApply(Board));
    TestFalse(TEXT("a position off the board is not"), MakeAt<FPlace>(0, 9, 9).CanApply(Board));

    // The one placement check, by position (what the client's hover uses)
    TestTrue(TEXT("free position is placeable"), FPlace::IsTilePlaceableAt(Board, FGridPosition(0, 0)));
    TestFalse(TEXT("occupied position is not"), FPlace::IsTilePlaceableAt(Board, FGridPosition(1, 1)));

    FConnectItTouchedPositions Touched;
    FConnectItBoardChangeEvent Event;
    Free.Apply(Board, Touched, &Event);
    TestEqual(TEXT("the piece is placed"), TileAt(Board, 0, 0).FactionPiece, 0);
    TestTrue(TEXT("exactly the placed position is reported"),
        Touched.Num() == 1 && Touched[0] == FGridPosition(0, 0));
    TestEqual(TEXT("applying an operation does not score"), Board.GetScore(0), 0.f);
    TestTrue(TEXT("the event says a piece was placed, where and by whom"),
        Event.bPiecePlaced && Event.PlacedPosition == FGridPosition(0, 0) && Event.PlacingFactionSlot == 0);

    TestEqual(TEXT("its request type is Place Piece"),
        Free.GetRequestType(), FGameplayTag(ConnectIt_Game_PlacePiece));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItSwapOperationTest,
    "ConnectIt.AI.MinMax.SwapOperation", TestFlags)

bool FConnectItSwapOperationTest::RunTest(const FString& Parameters)
{
    // Faction 0 has (3,0) (3,1) (3,2) and a stray piece at (0,0); faction 1
    // sits on (3,3). Swapping the stray with it completes faction 0's line.
    FConnectItBoardState Board = MakeBoard(7);
    Place(Board, 3, 0, 0);
    Place(Board, 3, 1, 0);
    Place(Board, 3, 2, 0);
    Place(Board, 0, 0, 0);
    Place(Board, 3, 3, 1);
    Place(Board, 6, 6, 1);

    const auto Swap = [](int32 Faction, FGridPosition A, FGridPosition B)
    {
        FConnectItBoardOperation_SwapPieces Operation;
        Operation.Faction = Faction;
        Operation.PositionA = A;
        Operation.PositionB = B;
        return Operation;
    };

    const FGridPosition Stray(0, 0);
    const FGridPosition Enemy(3, 3);
    const FConnectItBoardOperation_SwapPieces Trade = Swap(0, Stray, Enemy);
    TestTrue(TEXT("own piece for an enemy piece is valid"), Trade.CanApply(Board));
    TestTrue(TEXT("...for either faction involved"), Swap(1, Stray, Enemy).CanApply(Board));
    TestFalse(TEXT("two own pieces is not"), Swap(0, Stray, FGridPosition(3, 0)).CanApply(Board));
    TestFalse(TEXT("two enemy pieces is not"), Swap(0, Enemy, FGridPosition(6, 6)).CanApply(Board));
    TestFalse(TEXT("an empty tile is not"), Swap(0, Stray, FGridPosition(5, 5)).CanApply(Board));
    TestFalse(TEXT("a position off the board is not"), Swap(0, Stray, FGridPosition(9, 9)).CanApply(Board));

    FConnectItTouchedPositions Touched;
    FConnectItBoardChangeEvent Event;
    Trade.Apply(Board, Touched, &Event);
    TestEqual(TEXT("faction 1's piece is now on (0,0)"), TileAt(Board, 0, 0).FactionPiece, 1);
    TestEqual(TEXT("faction 0's piece is now on (3,3)"), TileAt(Board, 3, 3).FactionPiece, 0);
    TestTrue(TEXT("both positions are reported"), Touched.Num() == 2);
    TestEqual(TEXT("applying an operation does not score"), Board.GetScore(0), 0.f);
    TestTrue(TEXT("the event says which two were swapped"),
        Event.bPiecesSwapped && Event.SwapPositionA == Stray && Event.SwapPositionB == Enemy);

    // The separate after-move step is what scores the completed line
    const FConnectItRuleSet Rules;
    TArray<FConnectItScoringConfiguration> Configurations;
    const float Points = Rules.ResolveBoardChange(Board, Touched, &Configurations);
    TestEqual(TEXT("the completed line scores for faction 0"), Board.GetScore(0), 4.f);
    TestEqual(TEXT("faction 1 scores nothing"), Board.GetScore(1), 0.f);
    TestEqual(TEXT("the points are returned"), Points, 4.f);
    if (TestEqual(TEXT("one thing scored"), Configurations.Num(), 1))
    {
        TestEqual(TEXT("...for faction 0"), Configurations[0].FactionSlot, 0);
        TestEqual(TEXT("...worth 4 points"), Configurations[0].Points, 4.f);
        TestEqual(TEXT("...over 4 tiles"), Configurations[0].Positions.Num(), 4);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItScoringConfigurationsTest,
    "ConnectIt.AI.MinMax.ScoringConfigurationsPerLine", TestFlags)

bool FConnectItScoringConfigurationsTest::RunTest(const FString& Parameters)
{
    // A row and a column of three, both missing (3,3): one placement
    // completes two lines at once.
    const auto MakeCross = []()
    {
        FConnectItBoardState Board = MakeBoard(7);
        Place(Board, 3, 0, 0); Place(Board, 3, 1, 0); Place(Board, 3, 2, 0);
        Place(Board, 0, 3, 0); Place(Board, 1, 3, 0); Place(Board, 2, 3, 0);
        return Board;
    };

    const FConnectItRuleSet Rules;
    const FConnectItBoardOperation_PlacePiece Completing = MakeAt<FConnectItBoardOperation_PlacePiece>(0, 3, 3);

    FConnectItBoardState Board = MakeCross();
    FConnectItTouchedPositions Touched;
    Completing.Apply(Board, Touched, nullptr);

    TArray<FConnectItScoringConfiguration> Configurations;
    const float Points = Rules.ResolveBoardChange(Board, Touched, &Configurations);

    if (!TestEqual(TEXT("each completed line is its own configuration"), Configurations.Num(), 2)) return false;

    float Sum = 0.f;
    for (const FConnectItScoringConfiguration& Configuration : Configurations)
    {
        TestEqual(TEXT("scored for the placing faction"), Configuration.FactionSlot, 0);
        TestEqual(TEXT("a line of four tiles"), Configuration.Positions.Num(), 4);
        TestTrue(TEXT("includes the completing tile"), Configuration.Positions.Contains(FGridPosition(3, 3)));
        Sum += Configuration.Points;
    }
    TestEqual(TEXT("configurations add up to the points returned"), Sum, Points);
    TestEqual(TEXT("...and to the score gained"), Board.GetScore(0), Points);

    // The search asks for no details and gets the same result
    FConnectItBoardState Quiet = MakeCross();
    FConnectItTouchedPositions QuietTouched;
    Completing.Apply(Quiet, QuietTouched, nullptr);
    TestEqual(TEXT("same points without asking for configurations"),
        Rules.ResolveBoardChange(Quiet, QuietTouched), Points);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItNoOperationNoMovesTest,
    "ConnectIt.AI.MinMax.NoOperationNoMoves", TestFlags)

bool FConnectItNoOperationNoMovesTest::RunTest(const FString& Parameters)
{
    // Side 0 was granted no request types; side 1 can place
    const FConnectItBoardState Board = MakeBoard(5);
    const FConnectItMinMaxRules Rules(Board, RulesWonAt(100.f),
        DefaultEvaluationWeights(), DefaultOrderingWeights(), FGameplayTagContainer(), PlaceOnly());

    TArray<FConnectItMinMaxMove> Moves;
    Rules.GenerateMoves(Rules.MakeRoot(Board, 0), Moves);
    TestEqual(TEXT("a side with no operations has no moves"), Moves.Num(), 0);

    Rules.GenerateMoves(Rules.MakeRoot(Board, 1), Moves);
    TestEqual(TEXT("the side that can place has one per tile"), Moves.Num(), 25);
    return true;
}

// ---------------------------------------------------------------------------
// The operations nothing in the search uses. Force Place, Capture, Remove,
// Destroy Tile Multiplier and Toggle Tile Active have no sender in the game
// yet either, so these tests are their only exercise.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItForcePlaceOperationTest,
    "ConnectIt.Board.Operations.ForcePlacePiece", TestFlags)

bool FConnectItForcePlaceOperationTest::RunTest(const FString& Parameters)
{
    FConnectItBoardState Board = MakeBoard(5);
    Place(Board, 1, 1, 1);
    Board.GetTileDataMutable(FGridPosition(2, 2))->bIsActive = false;

    using FForce = FConnectItBoardOperation_ForcePlacePiece;
    TestTrue(TEXT("an occupied tile is allowed"), MakeAt<FForce>(0, 1, 1).CanApply(Board));
    TestTrue(TEXT("an inactive tile is allowed"), MakeAt<FForce>(0, 2, 2).CanApply(Board));
    TestFalse(TEXT("a position off the board is not"), MakeAt<FForce>(0, 9, 9).CanApply(Board));

    FConnectItTouchedPositions Touched;
    FConnectItBoardChangeEvent Event;
    MakeAt<FForce>(0, 1, 1).Apply(Board, Touched, &Event);
    TestEqual(TEXT("the piece there is replaced"), TileAt(Board, 1, 1).FactionPiece, 0);
    TestTrue(TEXT("the position is reported for scoring"),
        Touched.Num() == 1 && Touched[0] == FGridPosition(1, 1));
    TestTrue(TEXT("visuals see it as a placement"),
        Event.bPiecePlaced && Event.PlacedPosition == FGridPosition(1, 1) && Event.PlacingFactionSlot == 0);
    TestEqual(TEXT("the tile array did not grow"), Board.NumTiles(), 25);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItCaptureOperationTest,
    "ConnectIt.Board.Operations.CapturePiece", TestFlags)

bool FConnectItCaptureOperationTest::RunTest(const FString& Parameters)
{
    FConnectItBoardState Board = MakeBoard(5);
    Place(Board, 1, 1, 1);
    Place(Board, 2, 2, 0);

    using FCapture = FConnectItBoardOperation_CapturePiece;
    TestTrue(TEXT("an enemy piece can be captured"), MakeAt<FCapture>(0, 1, 1).CanApply(Board));
    TestFalse(TEXT("an own piece cannot"), MakeAt<FCapture>(0, 2, 2).CanApply(Board));
    TestFalse(TEXT("an empty tile cannot"), MakeAt<FCapture>(0, 3, 3).CanApply(Board));

    FConnectItTouchedPositions Touched;
    FConnectItBoardChangeEvent Event;
    MakeAt<FCapture>(0, 1, 1).Apply(Board, Touched, &Event);
    TestEqual(TEXT("the piece changes owner"), TileAt(Board, 1, 1).FactionPiece, 0);
    TestTrue(TEXT("the position is reported for scoring"),
        Touched.Num() == 1 && Touched[0] == FGridPosition(1, 1));
    TestTrue(TEXT("the event names both factions"),
        Event.bPieceCaptured && Event.CapturedPosition == FGridPosition(1, 1)
        && Event.CapturingFactionSlot == 0 && Event.PreviousFactionSlot == 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItShiftOperationTest,
    "ConnectIt.Board.Operations.Shift", TestFlags)

bool FConnectItShiftOperationTest::RunTest(const FString& Parameters)
{
    // Row Y = 0 of a 5-wide board: pieces on X = 0 (faction 0) and X = 1
    // (faction 1), a x3 multiplier on X = 4, and X = 2 can't shift.
    FConnectItBoardState Board = MakeBoard(5);
    Place(Board, 0, 0, 0);
    Place(Board, 1, 0, 1);
    Board.GetTileDataMutable(FGridPosition(2, 0))->bCanShift = false;
    Board.GetTileDataMutable(FGridPosition(4, 0))->Multiplier = 3.f;

    FConnectItBoardOperation_Shift Shift;
    Shift.Faction = 0;
    Shift.Direction = EGridDirection::Right;
    for (int32 X = 0; X < 5; X++)
    {
        Shift.Positions.Add(FGridPosition(X, 0));
    }

    TestTrue(TEXT("a line with enough shiftable tiles is valid"), Shift.CanApply(Board));

    FConnectItBoardOperation_Shift TooShort = Shift;
    TooShort.Positions.SetNum(1);
    TestFalse(TEXT("a single position is not"), TooShort.CanApply(Board));

    FConnectItBoardOperation_Shift OffBoard = Shift;
    OffBoard.Positions.Add(FGridPosition(9, 9));
    TestFalse(TEXT("a position off the board is not"), OffBoard.CanApply(Board));

    FConnectItTouchedPositions Touched;
    FConnectItBoardChangeEvent Event;
    Shift.Apply(Board, Touched, &Event);

    // Shiftable tiles are X = 0, 1, 3, 4; each takes the previous one's data
    // and the last wraps to the first
    TestEqual(TEXT("X=1 now holds faction 0's piece"), TileAt(Board, 1, 0).FactionPiece, 0);
    TestEqual(TEXT("X=3 now holds faction 1's piece (it jumped the fixed tile)"), TileAt(Board, 3, 0).FactionPiece, 1);
    TestEqual(TEXT("the far end's multiplier wrapped to X=0"), TileAt(Board, 0, 0).Multiplier, 3.f);
    TestEqual(TEXT("X=0 is now empty"), TileAt(Board, 0, 0).FactionPiece, -1);
    TestTrue(TEXT("the unshiftable tile kept its own data"),
        TileAt(Board, 2, 0).FactionPiece == -1 && !TileAt(Board, 2, 0).bCanShift);

    TestTrue(TEXT("positions that now hold a piece are reported for scoring"),
        Touched.Num() == 2 && Touched.Contains(FGridPosition(1, 0)) && Touched.Contains(FGridPosition(3, 0)));
    TestTrue(TEXT("the event describes the shift"),
        Event.bBoardShifted && Event.ShiftDirection == EGridDirection::Right
        && Event.ShiftAnchorPosition == FGridPosition(0, 0));
    TestTrue(TEXT("...with a from/to pair per tile that moved (not the fixed one)"),
        Event.ShiftStartPositions.Num() == 4 && Event.ShiftEndPositions.Num() == 4
        && !Event.ShiftEndPositions.Contains(FGridPosition(2, 0)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItRemoveOperationTest,
    "ConnectIt.Board.Operations.RemovePiece", TestFlags)

bool FConnectItRemoveOperationTest::RunTest(const FString& Parameters)
{
    FConnectItBoardState Board = MakeBoard(5);
    Place(Board, 1, 1, 1);

    using FRemove = FConnectItBoardOperation_RemovePiece;
    const FRemove Remove = MakeAt<FRemove>(0, 1, 1);
    TestTrue(TEXT("an occupied tile can be cleared"), Remove.CanApply(Board));
    TestFalse(TEXT("an empty tile cannot"), MakeAt<FRemove>(0, 2, 2).CanApply(Board));

    FRemove Delayed = Remove;
    Delayed.DelayTurns = 2;
    TestFalse(TEXT("a delayed removal is refused (not supported yet)"), Delayed.CanApply(Board));

    FConnectItTouchedPositions Touched;
    FConnectItBoardChangeEvent Event;
    Remove.Apply(Board, Touched, &Event);
    TestTrue(TEXT("the piece is gone"),
        TileAt(Board, 1, 1).FactionPiece == -1 && !TileAt(Board, 1, 1).bIsOccupied);
    TestEqual(TEXT("nothing is reported for scoring"), Touched.Num(), 0);
    TestTrue(TEXT("the event says whose piece was removed"),
        Event.bPieceRemoved && Event.RemovedPosition == FGridPosition(1, 1) && Event.RemovedFactionSlot == 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItDestroyMultiplierOperationTest,
    "ConnectIt.Board.Operations.DestroyTileMultiplier", TestFlags)

bool FConnectItDestroyMultiplierOperationTest::RunTest(const FString& Parameters)
{
    FConnectItBoardState Board = MakeBoard(5);
    Board.GetTileDataMutable(FGridPosition(1, 1))->Multiplier = 3.f;

    using FDestroy = FConnectItBoardOperation_DestroyTileMultiplier;
    TestTrue(TEXT("a tile with a multiplier is valid"), MakeAt<FDestroy>(0, 1, 1).CanApply(Board));
    TestFalse(TEXT("a tile at x1 is not"), MakeAt<FDestroy>(0, 2, 2).CanApply(Board));
    TestFalse(TEXT("a position off the board is not"), MakeAt<FDestroy>(0, 9, 9).CanApply(Board));

    FConnectItTouchedPositions Touched;
    FConnectItBoardChangeEvent Event;
    MakeAt<FDestroy>(0, 1, 1).Apply(Board, Touched, &Event);
    TestEqual(TEXT("the multiplier is back to 1"), TileAt(Board, 1, 1).Multiplier, 1.f);
    TestEqual(TEXT("nothing is reported for scoring"), Touched.Num(), 0);
    TestTrue(TEXT("the event says where"),
        Event.bTileMultiplierDestroyed && Event.MultiplierDestroyedPosition == FGridPosition(1, 1));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItToggleActiveOperationTest,
    "ConnectIt.Board.Operations.ToggleTileActive", TestFlags)

bool FConnectItToggleActiveOperationTest::RunTest(const FString& Parameters)
{
    FConnectItBoardState Board = MakeBoard(5);

    using FToggle = FConnectItBoardOperation_ToggleTileActive;
    const FToggle Toggle = MakeAt<FToggle>(0, 1, 1);
    TestTrue(TEXT("any tile on the board is valid"), Toggle.CanApply(Board));
    TestFalse(TEXT("a position off the board is not"), MakeAt<FToggle>(0, 9, 9).CanApply(Board));

    FConnectItTouchedPositions Touched;
    FConnectItBoardChangeEvent Event;
    Toggle.Apply(Board, Touched, &Event);
    TestFalse(TEXT("an active tile becomes inactive"), TileAt(Board, 1, 1).bIsActive);
    TestEqual(TEXT("nothing is reported for scoring"), Touched.Num(), 0);
    TestTrue(TEXT("the event says where and the new state"),
        Event.bTileActiveToggled && Event.ToggledPosition == FGridPosition(1, 1)
        && !Event.bToggledPositionNowActive);

    FConnectItBoardChangeEvent Again;
    Toggle.Apply(Board, Touched, &Again);
    TestTrue(TEXT("and back again"), TileAt(Board, 1, 1).bIsActive && Again.bToggledPositionNowActive);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
