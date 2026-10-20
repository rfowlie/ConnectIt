// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "MinMax/ConnectIt_MinMaxRules.h"
#include "AI/ConnectIt_AIStrategy_MinMax.h"
#include "AI/ConnectIt_AIDirector.h"
#include "Framework/Data/ConnectIt_MatchPreset.h"
#include "AI/ConnectIt_AICoordinator.h"
#include "AI/ConnectIt_MinMaxSearch.h"
#include "AI/ConnectIt_AISelector.h"
#include "ConnectIt_MinMaxTestTerms.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Board/Rules/ConnectIt_MatchConditionAsset.h"
#include "Framework/Library/ConnectIt_BoardStateLibrary.h"
#include "Board/Rules/ConnectIt_LineScoringRule.h"
#include "Board/Rules/ConnectIt_ScoreThresholdCondition.h"
#include "Board/Rules/ConnectIt_PieceCountCondition.h"
#include "Board/Rules/ConnectIt_NotEnoughPiecesToScoreCondition.h"
#include "Board/Operations/ConnectIt_BoardOperations.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "ConnectIt_GameplayTags.h"
#include "Misc/DataValidation.h"

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
        return GetDefault<UConnectIt_AIStrategy_MinMax>()->Search.EvaluationWeights;
    }

    const FConnectItMinMaxOrderingWeights& DefaultOrderingWeights()
    {
        return GetDefault<UConnectIt_AIStrategy_MinMax>()->Search.OrderingWeights;
    }

    template<typename TCondition>
    void AddCondition(FConnectItMatchConditionGroup& Group, const TCondition& Condition)
    {
        Group.Conditions.Add(TInstancedStruct<FConnectItMatchCondition>::Make<TCondition>(Condition));
    }

    FConnectItMatchCondition_ScoreThreshold ScoreAtLeast(float Threshold)
    {
        FConnectItMatchCondition_ScoreThreshold Condition;
        Condition.WinScoreThreshold = Threshold;
        return Condition;
    }

    FConnectItMatchCondition_PieceCount PiecesIs(EConnectItPieceCountComparison Comparison, int32 Count)
    {
        FConnectItMatchCondition_PieceCount Condition;
        Condition.Comparison = Comparison;
        Condition.Count = Count;
        return Condition;
    }

    // A new, empty win group at the end of Groups (valid until Groups changes)
    FConnectItMatchConditionGroup& AddGroup(TArray<FConnectItMatchConditionGroup>& Groups, FName Name = NAME_None)
    {
        FConnectItMatchConditionGroup& Group = Groups.AddDefaulted_GetRef();
        Group.Name = Name;
        return Group;
    }

    // The classic rule set (lines of 4) won at Threshold: one player group. A
    // fresh rule set has no win groups, so every test that needs one starts here.
    FConnectItRuleSet RulesWonAt(float Threshold)
    {
        FConnectItRuleSet Rules;
        AddCondition(AddGroup(Rules.PlayerWinGroups), ScoreAtLeast(Threshold));
        return Rules;
    }

    // Win state is persisted on the board (completed groups), so a board built
    // by hand has to be stamped before anything is "won". These stamp a copy.
    int32 WinnerOf(const FConnectItRuleSet& Rules, FConnectItBoardState Board)
    {
        Rules.StampWinState(Board);
        return Board.bGameOver ? Board.WinningFactionSlot : static_cast<int32>(INDEX_NONE);
    }

    bool HasWon(const FConnectItRuleSet& Rules, FConnectItBoardState Board, int32 Faction)
    {
        Rules.StampWinState(Board);
        return Rules.HasFactionWon(Board, Faction);
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

    // The event at Index of a change's list, if it is a TEvent
    template<typename TEvent>
    const TEvent* EventAt(const FConnectItBoardChangeEvent& Change, int32 Index)
    {
        return Change.Events.IsValidIndex(Index) ? Change.Events[Index].GetPtr<TEvent>() : nullptr;
    }

    // The change's one and only event, if it is a TEvent
    template<typename TEvent>
    const TEvent* OnlyEvent(const FConnectItBoardChangeEvent& Change)
    {
        return Change.Events.Num() == 1 ? EventAt<TEvent>(Change, 0) : nullptr;
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
    AddCondition(AddGroup(CornerRules.PlayerWinGroups), FConnectItMatchCondition_TestCorner());

    const FConnectItMinMaxRules Rules(Board, CornerRules,
        DefaultEvaluationWeights(), DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());

    const auto Result = FConnectItMinMaxSearch::Run(Rules, Rules.MakeRoot(Board, 0), Depth(2));

    if (!TestTrue(TEXT("search returned moves"), Result.RootScores.Num() > 0)) return false;
    TestTrue(TEXT("the AI takes the winning corner"), IsPosition(Result.RootScores[0].Move, 0, 0));
    TestTrue(TEXT("and scores it as a win"),
        Result.RootScores[0].Score > FConnectItMinMaxRules::WinValue / 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItPieceCountWinTest,
    "ConnectIt.Board.Conditions.PieceCount", TestFlags)

bool FConnectItPieceCountWinTest::RunTest(const FString& Parameters)
{
    FConnectItBoardState Board = MakeBoard(5);
    Place(Board, 0, 0, 0);
    Place(Board, 1, 0, 0);
    Place(Board, 4, 4, 1);
    Board.ScoreBoard = { 0.f, 0.f };
    Board.StartingPieces = { 2, 1 };
    const FConnectItRuleSet NoRules;
    const FConnectItConditionContext Ctx{ Board, NoRules };

    // Faction 0 holds 2 pieces, faction 1 holds 1
    TestFalse(TEXT("Equal 0: pieces left"), PiecesIs(EConnectItPieceCountComparison::Equal, 0).IsMetFor(Ctx, 0));
    TestTrue(TEXT("Equal 2 is met by faction 0"), PiecesIs(EConnectItPieceCountComparison::Equal, 2).IsMetFor(Ctx, 0));
    TestFalse(TEXT("...and not by faction 1"), PiecesIs(EConnectItPieceCountComparison::Equal, 2).IsMetFor(Ctx, 1));
    TestTrue(TEXT("Less Than 3: 2 < 3"), PiecesIs(EConnectItPieceCountComparison::LessThan, 3).IsMetFor(Ctx, 0));
    TestFalse(TEXT("Less Than 2: 2 < 2 is false"), PiecesIs(EConnectItPieceCountComparison::LessThan, 2).IsMetFor(Ctx, 0));
    TestTrue(TEXT("Greater Than 1: 2 > 1"), PiecesIs(EConnectItPieceCountComparison::GreaterThan, 1).IsMetFor(Ctx, 0));
    TestFalse(TEXT("Greater Than 2: 2 > 2 is false"), PiecesIs(EConnectItPieceCountComparison::GreaterThan, 2).IsMetFor(Ctx, 0));
    TestFalse(TEXT("an unknown faction can't win"), PiecesIs(EConnectItPieceCountComparison::Equal, 0).IsMetFor(Ctx, 5));

    // Progress is measured from the starting count the board remembers
    const FConnectItMatchCondition_PieceCount ClearAll = PiecesIs(EConnectItPieceCountComparison::Equal, 0);
    TestEqual(TEXT("no progress yet (2 of 2 left)"), ClearAll.GetProgress(Ctx, 0), 0.f);
    Board.GetTileDataMutable(FGridPosition(0, 0))->SetFactionPiece(-1);
    TestEqual(TEXT("half way (1 of 2 left)"), ClearAll.GetProgress(Ctx, 0), 0.5f);
    Board.GetTileDataMutable(FGridPosition(1, 0))->SetFactionPiece(-1);
    TestTrue(TEXT("none left: met with no score at all"), ClearAll.IsMetFor(Ctx, 0));
    TestEqual(TEXT("full progress on the win"), ClearAll.GetProgress(Ctx, 0), 1.f);

    // Less Than 1 is the same goal as Equal 0, and Greater Than counts up
    TestEqual(TEXT("Less Than 1 aims at 0"),
        PiecesIs(EConnectItPieceCountComparison::LessThan, 1).GetProgress(Ctx, 1), 0.f);
    FConnectItBoardState Growing = MakeBoard(5);
    Place(Growing, 0, 0, 0);
    Place(Growing, 1, 0, 0);
    Growing.ScoreBoard = { 0.f, 0.f };
    Growing.StartingPieces = { 1, 0 };
    const FConnectItConditionContext GrowingCtx{ Growing, NoRules };
    TestEqual(TEXT("Greater Than 2 aims at 3: from 1 to 2 is half way"),
        PiecesIs(EConnectItPieceCountComparison::GreaterThan, 2).GetProgress(GrowingCtx, 0), 0.5f);

    // No starting count on the board: progress is 0 until met
    Growing.StartingPieces.Empty();
    TestEqual(TEXT("no baseline: 0"),
        PiecesIs(EConnectItPieceCountComparison::GreaterThan, 2).GetProgress(GrowingCtx, 0), 0.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItNotEnoughPiecesTest,
    "ConnectIt.Board.Conditions.NotEnoughPiecesToScore", TestFlags)

bool FConnectItNotEnoughPiecesTest::RunTest(const FString& Parameters)
{
    // Lines(4): a faction needs 4 pieces to ever score
    FConnectItRuleSet Rules;
    FConnectItBoardState Board = MakeBoard(5);
    Board.ScoreBoard = { 0.f, 0.f };
    Place(Board, 0, 0, 0);
    Place(Board, 1, 0, 0);
    Place(Board, 2, 0, 0);
    const FConnectItConditionContext Ctx{ Board, Rules };
    const FConnectItMatchCondition_NotEnoughPiecesToScore Stuck;

    TestEqual(TEXT("Lines says how many a score needs"), Rules.GetScoringRule()->GetMinPiecesToScore(), 4);
    TestTrue(TEXT("3 pieces can't make a line of 4"), Stuck.IsMetFor(Ctx, 0));
    Place(Board, 3, 0, 0);
    TestFalse(TEXT("4 pieces can"), Stuck.IsMetFor(Ctx, 0));
    TestTrue(TEXT("no pieces at all counts too"), Stuck.IsMetFor(Ctx, 1));
    TestFalse(TEXT("an unknown faction never"), Stuck.IsMetFor(Ctx, 5));

    // The minimum comes from the rule: Lines(5) needs 5
    FConnectItRuleSet LongLines;
    LongLines.ScoringRule.GetMutable<FConnectItScoringRule_Lines>().ConnectLength = 5;
    TestTrue(TEXT("4 pieces can't make a line of 5"), Stuck.IsMetFor(FConnectItConditionContext{ Board, LongLines }, 0));

    // A scoring rule that doesn't say: never met, and validation says so
    FConnectItRuleSet NoMinimum;
    NoMinimum.ScoringRule = TInstancedStruct<FConnectItScoringRule>::Make<FConnectItScoringRule>();
    TestFalse(TEXT("no minimum: never met"), Stuck.IsMetFor(FConnectItConditionContext{ Board, NoMinimum }, 1));
    TArray<FText> Warnings;
    Stuck.GetValidationWarnings(&NoMinimum, Warnings);
    TestEqual(TEXT("...and validation warns"), Warnings.Num(), 1);
    Warnings.Reset();
    Stuck.GetValidationWarnings(&Rules, Warnings);
    TestEqual(TEXT("no warning under Lines"), Warnings.Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItRuleSetWinConditionsTest,
    "ConnectIt.Board.Conditions.RuleSet", TestFlags)

bool FConnectItRuleSetWinConditionsTest::RunTest(const FString& Parameters)
{
    // A swap-puzzle level: the player (faction 0) wins by clearing its pieces
    // AFTER scoring 4 (one group); the AI (faction 1) by reaching 8 points.
    FConnectItRuleSet Rules;
    {
        FConnectItMatchConditionGroup& Goal = AddGroup(Rules.PlayerWinGroups);
        AddCondition(Goal, PiecesIs(EConnectItPieceCountComparison::Equal, 0));
        AddCondition(Goal, ScoreAtLeast(4.f));
    }
    AddCondition(AddGroup(Rules.AIWinGroups), ScoreAtLeast(8.f));
    Rules.AIFaction = 1;

    FConnectItBoardState Board = MakeBoard(5);
    Place(Board, 0, 0, 0);
    Board.StartingPieces = { 1, 0 };
    Board.ScoreBoard = { 4.f, 0.f };
    TestEqual(TEXT("nobody has won yet"), WinnerOf(Rules, Board), static_cast<int32>(INDEX_NONE));

    // The player's score alone is not enough: a piece is still on the board
    Board.ScoreBoard = { 50.f, 0.f };
    TestFalse(TEXT("score without clearing the pieces"), HasWon(Rules, Board, 0));

    // ...and clearing the pieces alone is not enough either (the 'win without
    // scoring' hole the second condition closes)
    Board.GetTileDataMutable(FGridPosition(0, 0))->SetFactionPiece(-1);
    Board.ScoreBoard = { 0.f, 0.f };
    TestFalse(TEXT("pieces cleared but nothing scored"), HasWon(Rules, Board, 0));
    Board.ScoreBoard = { 4.f, 2.f };
    TestEqual(TEXT("both hold: the player wins"), WinnerOf(Rules, Board), 0);

    // The AI reaching 8 wins by its own list
    Place(Board, 0, 0, 0);
    Board.ScoreBoard = { 4.f, 8.f };
    TestEqual(TEXT("the AI wins at its threshold"), WinnerOf(Rules, Board), 1);

    // Both at once: the lowest slot wins
    Board.GetTileDataMutable(FGridPosition(0, 0))->SetFactionPiece(-1);
    Board.ScoreBoard = { 4.f, 9.f };
    TestEqual(TEXT("both satisfied: lowest slot"), WinnerOf(Rules, Board), 0);

    // The AI's 8 belongs to the AI: the player reaching 8 does not win by it
    FConnectItBoardState Other = MakeBoard(5);
    Place(Other, 0, 0, 0);
    Other.ScoreBoard = { 8.f, 0.f };
    TestFalse(TEXT("the AI's target belongs to the AI"), HasWon(Rules, Other, 0));

    // Progress: a group is the average of its conditions; the AI reads its own
    FConnectItBoardState Progress = MakeBoard(5);
    Place(Progress, 0, 0, 0);
    Progress.StartingPieces = { 1, 0 };
    Progress.ScoreBoard = { 2.f, 4.f };
    TestEqual(TEXT("player: pieces 0 and score 2 of 4 -> 0.25"), Rules.GetProgress(Progress, 0), 0.25f);
    TestEqual(TEXT("AI: score 4 of 8"), Rules.GetProgress(Progress, 1), 0.5f);

    // Target score: the player's first group has a score-based condition; a
    // menu override reaches the final group of both lists
    TestEqual(TEXT("the target comes from the player's list"), Rules.GetTargetScore(), 4.f);
    TestTrue(TEXT("the override applies"), Rules.SetTargetScore(20.f));
    float AITarget = 0.f;
    TestTrue(TEXT("...to the AI's list too"), Rules.AIWinGroups.Last().GetTargetScore(AITarget));
    TestEqual(TEXT("...with the new value"), AITarget, 20.f);
    TestEqual(TEXT("...and the player's"), Rules.GetTargetScore(), 20.f);

    // No fallback between sides: an AI with no goals of its own cannot win
    FConnectItRuleSet NoAIGoals = RulesWonAt(8.f);
    NoAIGoals.AIFaction = 1;
    Other.ScoreBoard = { 0.f, 100.f };
    TestEqual(TEXT("empty AI list: the AI can't win"),
        WinnerOf(NoAIGoals, Other), static_cast<int32>(INDEX_NONE));

    // No AI in the match (Online): the player's list applies to every faction
    FConnectItRuleSet Online = RulesWonAt(15.f);
    AddCondition(AddGroup(Online.AIWinGroups), ScoreAtLeast(5.f));
    FConnectItBoardState Humans = MakeBoard(5);
    Place(Humans, 0, 0, 0);
    Humans.ScoreBoard = { 0.f, 20.f };
    Online.AIFaction = INDEX_NONE;
    TestEqual(TEXT("no AI slot: faction 1 wins by the player's list (15)"), WinnerOf(Online, Humans), 1);
    Humans.ScoreBoard = { 0.f, 10.f };
    TestEqual(TEXT("...and 10 is not enough for it"), WinnerOf(Online, Humans), static_cast<int32>(INDEX_NONE));
    Online.AIFaction = 1;
    TestEqual(TEXT("with an AI slot, the AI's own list (5) applies instead"), WinnerOf(Online, Humans), 1);

    // A group with a cleared slot can't be satisfied, rather than skipping it
    FConnectItRuleSet HalfSet = RulesWonAt(1.f);
    HalfSet.PlayerWinGroups[0].Conditions.Add(TInstancedStruct<FConnectItMatchCondition>());
    Other.ScoreBoard = { 50.f, 0.f };
    TestFalse(TEXT("an empty slot in the group blocks the win"), HasWon(HalfSet, Other, 0));

    // A group with no conditions is never completed, so it blocks the ones after it
    FConnectItRuleSet Blocked;
    AddGroup(Blocked.PlayerWinGroups, TEXT("Nothing"));
    AddCondition(AddGroup(Blocked.PlayerWinGroups), ScoreAtLeast(1.f));
    TestEqual(TEXT("an empty group blocks the list"), WinnerOf(Blocked, Other), static_cast<int32>(INDEX_NONE));

    // No win groups at all: nobody wins, nothing to show
    const FConnectItRuleSet Empty;
    Other.ScoreBoard = { 1000.f, 1000.f };
    TestEqual(TEXT("no win groups: nobody wins"), WinnerOf(Empty, Other), static_cast<int32>(INDEX_NONE));
    TestEqual(TEXT("...no target score"), Empty.GetTargetScore(), 0.f);
    TestEqual(TEXT("...no board target score"), Empty.GetTargetScore(Other), 0.f);
    TestEqual(TEXT("...no progress"), Empty.GetProgress(Other, 0), 0.f);
    FConnectItRuleSet EmptyCopy = Empty;
    TestFalse(TEXT("...and the override has nothing to set"), EmptyCopy.SetTargetScore(5.f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItWinStagesTest,
    "ConnectIt.Board.Conditions.Stages", TestFlags)

bool FConnectItWinStagesTest::RunTest(const FString& Parameters)
{
    // The player's goals, in order: score 4 ("Score"), THEN have no pieces
    // left ("Clear"). The AI has one goal: 10 points.
    FConnectItRuleSet Rules;
    AddCondition(AddGroup(Rules.PlayerWinGroups, TEXT("Score")), ScoreAtLeast(4.f));
    AddCondition(AddGroup(Rules.PlayerWinGroups, TEXT("Clear")), PiecesIs(EConnectItPieceCountComparison::Equal, 0));
    AddCondition(AddGroup(Rules.AIWinGroups, TEXT("AIGoal")), ScoreAtLeast(10.f));
    Rules.AIFaction = 1;

    FConnectItBoardState Board = MakeBoard(5);
    Place(Board, 0, 0, 0);
    Place(Board, 4, 4, 1);
    Board.StartingPieces = { 1, 1 };
    Board.ScoreBoard = { 0.f, 0.f };

    // Nothing met yet
    FConnectItBoardChangeEvent Change;
    Rules.StampWinState(Board, &Change);
    TestEqual(TEXT("no checkpoint events yet"), Change.Events.Num(), 0);
    TestEqual(TEXT("no group completed"), Board.GroupsCompleted[0], 0);
    TestEqual(TEXT("target score is the first group's"), Rules.GetTargetScore(Board), 4.f);
    TestFalse(TEXT("not over"), Board.bGameOver);

    // Group 0 met: a checkpoint, not a win
    Board.ScoreBoard = { 4.f, 0.f };
    Change = FConnectItBoardChangeEvent();
    Rules.StampWinState(Board, &Change);
    TestEqual(TEXT("first group completed"), Board.GroupsCompleted[0], 1);
    if (const auto* Checkpoint = OnlyEvent<FConnectItBoardEvent_WinCheckpointReached>(Change))
    {
        TestEqual(TEXT("checkpoint faction"), Checkpoint->Faction, 0);
        TestEqual(TEXT("checkpoint group index"), Checkpoint->GroupIndex, 0);
        TestEqual(TEXT("checkpoint group name"), Checkpoint->GroupName, FName(TEXT("Score")));
    }
    else
    {
        AddError(TEXT("expected exactly one WinCheckpointReached event"));
    }
    TestFalse(TEXT("a checkpoint is not a win"), Board.bGameOver);
    TestEqual(TEXT("progress: one group done, the second not started (0 of 1 pieces cleared)"),
        Rules.GetProgress(Board, 0), 0.5f);
    TestEqual(TEXT("the AI is untouched"), Board.GroupsCompleted[1], 0);

    // Completed groups stay completed, and don't fire again
    Change = FConnectItBoardChangeEvent();
    Rules.StampWinState(Board, &Change);
    TestEqual(TEXT("no second event for the same group"), Change.Events.Num(), 0);
    TestEqual(TEXT("still one group completed"), Board.GroupsCompleted[0], 1);

    // The last group is the win, and is not reported as a checkpoint
    Board.GetTileDataMutable(FGridPosition(0, 0))->SetFactionPiece(-1);
    Change = FConnectItBoardChangeEvent();
    Rules.StampWinState(Board, &Change);
    TestEqual(TEXT("both groups completed"), Board.GroupsCompleted[0], 2);
    TestEqual(TEXT("the last group is a win, not a checkpoint"), Change.Events.Num(), 0);
    TestTrue(TEXT("game over"), Board.bGameOver);
    TestEqual(TEXT("the player won"), Board.WinningFactionSlot, 0);
    TestEqual(TEXT("progress is full"), Rules.GetProgress(Board, 0), 1.f);

    // One move can complete several groups: one checkpoint event, then the win
    FConnectItBoardState Chained = MakeBoard(5);
    Place(Chained, 4, 4, 1);
    Chained.StartingPieces = { 1, 1 };
    Chained.ScoreBoard = { 4.f, 0.f };
    Change = FConnectItBoardChangeEvent();
    Rules.StampWinState(Chained, &Change);
    TestEqual(TEXT("both groups completed by one stamp"), Chained.GroupsCompleted[0], 2);
    TestEqual(TEXT("one checkpoint event (for the first group only)"), Change.Events.Num(), 1);
    TestTrue(TEXT("and the win"), Chained.bGameOver && Chained.WinningFactionSlot == 0);

    // A completed group is latched: its condition stopping to hold doesn't undo it
    FConnectItRuleSet Latch;
    AddCondition(AddGroup(Latch.PlayerWinGroups, TEXT("HavePieces")), PiecesIs(EConnectItPieceCountComparison::GreaterThan, 0));
    AddCondition(AddGroup(Latch.PlayerWinGroups, TEXT("Score")), ScoreAtLeast(4.f));
    FConnectItBoardState Held = MakeBoard(5);
    Place(Held, 0, 0, 0);
    Held.ScoreBoard = { 0.f, 0.f };
    Latch.StampWinState(Held);
    TestEqual(TEXT("first group completed while a piece is held"), Held.GroupsCompleted[0], 1);
    Held.GetTileDataMutable(FGridPosition(0, 0))->SetFactionPiece(-1);
    Held.ScoreBoard = { 4.f, 0.f };
    Latch.StampWinState(Held);
    TestEqual(TEXT("it stays complete after the piece is gone, and the second completes"), Held.GroupsCompleted[0], 2);
    TestTrue(TEXT("won"), Held.bGameOver);

    // Each side has its own list and its own progress: the AI's single group is a win at once
    FConnectItBoardState AIWins = MakeBoard(5);
    Place(AIWins, 0, 0, 0);
    AIWins.StartingPieces = { 1, 1 };
    AIWins.ScoreBoard = { 0.f, 10.f };
    Change = FConnectItBoardChangeEvent();
    Rules.StampWinState(AIWins, &Change);
    TestEqual(TEXT("the AI completed its only group"), AIWins.GroupsCompleted[1], 1);
    TestEqual(TEXT("the player is where they were"), AIWins.GroupsCompleted[0], 0);
    TestEqual(TEXT("a single-group list reports no checkpoint"), Change.Events.Num(), 0);
    TestTrue(TEXT("the AI won"), AIWins.bGameOver && AIWins.WinningFactionSlot == 1);

    // A menu target override changes each list's LAST group only
    FConnectItRuleSet Menu = Rules;
    AddCondition(AddGroup(Menu.AIWinGroups, TEXT("AIFinal")), ScoreAtLeast(30.f));
    TestTrue(TEXT("the override applies"), Menu.SetTargetScore(7.f));
    float Target = 0.f;
    TestTrue(TEXT("the player's first group is a checkpoint..."), Menu.PlayerWinGroups[0].GetTargetScore(Target));
    TestEqual(TEXT("...and keeps its designed target"), Target, 4.f);
    TestTrue(TEXT("the AI's first group too"), Menu.AIWinGroups[0].GetTargetScore(Target));
    TestEqual(TEXT("...keeps 10"), Target, 10.f);
    TestTrue(TEXT("the AI's last group has the override"), Menu.AIWinGroups.Last().GetTargetScore(Target));
    TestEqual(TEXT("...7"), Target, 7.f);
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
    const float Points = RuleSet.ApplyScoring(GameBoard, FGridPosition(3, 3), 0, true);

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
    const auto* Placed = OnlyEvent<FConnectItBoardEvent_PiecePlaced>(Event);
    TestTrue(TEXT("one Piece Placed event says where and by whom"),
        Placed && Placed->Position == FGridPosition(0, 0) && Placed->Faction == 0);

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
    const auto* Swapped = OnlyEvent<FConnectItBoardEvent_PiecesSwapped>(Event);
    TestTrue(TEXT("one Pieces Swapped event says which two"),
        Swapped && Swapped->PositionA == Stray && Swapped->PositionB == Enemy);

    // The separate after-move step is what scores the completed line
    const FConnectItRuleSet Rules;
    TestFalse(TEXT("a swap's arriving pieces don't survive scoring"), Trade.ArrivingPiecesSurviveScoring());
    TestTrue(TEXT("a placement's do"), FConnectItBoardOperation_PlacePiece().ArrivingPiecesSurviveScoring());

    const float Points = Rules.ResolveBoardChange(Board, Touched, Trade.ArrivingPiecesSurviveScoring(), &Event);
    TestEqual(TEXT("the completed line scores for faction 0"), Board.GetScore(0), 4.f);
    TestEqual(TEXT("the swapped-in piece is cleared with the line"), TileAt(Board, 3, 3).FactionPiece, -1);
    TestEqual(TEXT("the piece swapped the other way is untouched"), TileAt(Board, 0, 0).FactionPiece, 1);
    TestEqual(TEXT("faction 1 scores nothing"), Board.GetScore(1), 0.f);
    TestEqual(TEXT("the points are returned"), Points, 4.f);
    if (TestEqual(TEXT("the swap event is followed by one Scored event"), Event.Events.Num(), 2))
    {
        const auto* Scored = EventAt<FConnectItBoardEvent_Scored>(Event, 1);
        if (TestNotNull(TEXT("the second event is a Scored event"), Scored))
        {
            TestEqual(TEXT("...for faction 0"), Scored->Faction, 0);
            TestEqual(TEXT("...worth 4 points"), Scored->Points, 4.f);
            TestEqual(TEXT("...over 4 tiles"), Scored->Positions.Num(), 4);
            TestEqual(TEXT("...all 4 pieces cleared"), Scored->ClearedPositions.Num(), 4);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItScoredEventPerLineTest,
    "ConnectIt.Board.Events.ScoredEventPerLine", TestFlags)

bool FConnectItScoredEventPerLineTest::RunTest(const FString& Parameters)
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

    FConnectItBoardChangeEvent Change;
    const float Points = Rules.ResolveBoardChange(Board, Touched, true, &Change);

    if (!TestEqual(TEXT("each completed line is its own Scored event"), Change.Events.Num(), 2)) return false;

    float Sum = 0.f;
    for (int32 Index = 0; Index < Change.Events.Num(); Index++)
    {
        const auto* Scored = EventAt<FConnectItBoardEvent_Scored>(Change, Index);
        if (!TestNotNull(TEXT("a Scored event"), Scored)) return false;

        TestEqual(TEXT("scored for the placing faction"), Scored->Faction, 0);
        TestEqual(TEXT("a line of four tiles"), Scored->Positions.Num(), 4);
        TestTrue(TEXT("includes the completing tile"), Scored->Positions.Contains(FGridPosition(3, 3)));
        TestEqual(TEXT("clears the other three pieces"), Scored->ClearedPositions.Num(), 3);
        TestFalse(TEXT("...not the completing piece"), Scored->ClearedPositions.Contains(FGridPosition(3, 3)));
        Sum += Scored->Points;
    }
    TestEqual(TEXT("the completing piece is still on the board"), TileAt(Board, 3, 3).FactionPiece, 0);
    TestEqual(TEXT("the events add up to the points returned"), Sum, Points);
    TestEqual(TEXT("...and to the score gained"), Board.GetScore(0), Points);

    // The search asks for no details and gets the same result
    FConnectItBoardState Quiet = MakeCross();
    FConnectItTouchedPositions QuietTouched;
    Completing.Apply(Quiet, QuietTouched, nullptr);
    TestEqual(TEXT("same points without asking for events"),
        Rules.ResolveBoardChange(Quiet, QuietTouched, true), Points);

    // The same cross, completed by a piece that does NOT survive scoring
    // (what a swap's arriving piece does)
    FConnectItBoardState Cleared = MakeCross();
    FConnectItTouchedPositions ClearedTouched;
    Completing.Apply(Cleared, ClearedTouched, nullptr);
    FConnectItBoardChangeEvent ClearedChange;
    const float ClearedPoints = Rules.ResolveBoardChange(Cleared, ClearedTouched, false, &ClearedChange);

    TestEqual(TEXT("both lines still score the same points"), ClearedPoints, Points);
    TestEqual(TEXT("the completing tile ends up empty"), TileAt(Cleared, 3, 3).FactionPiece, -1);
    const auto* FirstLine = EventAt<FConnectItBoardEvent_Scored>(ClearedChange, 0);
    const auto* SecondLine = EventAt<FConnectItBoardEvent_Scored>(ClearedChange, 1);
    if (TestTrue(TEXT("two Scored events"), ClearedChange.Events.Num() == 2 && FirstLine && SecondLine))
    {
        TestEqual(TEXT("the first clears its whole line"), FirstLine->ClearedPositions.Num(), 4);
        TestEqual(TEXT("the second clears only what is left"), SecondLine->ClearedPositions.Num(), 3);
        TestFalse(TEXT("no position is cleared twice"), SecondLine->ClearedPositions.Contains(FGridPosition(3, 3)));
    }
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
    const auto* Placed = OnlyEvent<FConnectItBoardEvent_PiecePlaced>(Event);
    TestTrue(TEXT("visuals see it as a placement"),
        Placed && Placed->Position == FGridPosition(1, 1) && Placed->Faction == 0);
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
    const auto* Captured = OnlyEvent<FConnectItBoardEvent_PieceCaptured>(Event);
    TestTrue(TEXT("one Piece Captured event names both factions"),
        Captured && Captured->Position == FGridPosition(1, 1)
        && Captured->CapturingFaction == 0 && Captured->PreviousFaction == 1);
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
    const auto* Shifted = OnlyEvent<FConnectItBoardEvent_BoardShifted>(Event);
    if (!TestNotNull(TEXT("one Board Shifted event"), Shifted)) return false;
    TestTrue(TEXT("it describes the shift"),
        Shifted->Direction == EGridDirection::Right && Shifted->AnchorPosition == FGridPosition(0, 0));
    TestTrue(TEXT("...with a from/to pair per tile that moved (not the fixed one)"),
        Shifted->StartPositions.Num() == 4 && Shifted->EndPositions.Num() == 4
        && !Shifted->EndPositions.Contains(FGridPosition(2, 0)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItShiftStepsTest,
    "ConnectIt.Board.Operations.ShiftSteps", TestFlags)

bool FConnectItShiftStepsTest::RunTest(const FString& Parameters)
{
    // Row Y = 0 of a 5-wide board with distinct data per tile: a piece of
    // faction 0 on X = 0, faction 1 on X = 1, and a x3 multiplier on X = 4
    FConnectItBoardState Board = MakeBoard(5);
    Place(Board, 0, 0, 0);
    Place(Board, 1, 0, 1);
    Board.GetTileDataMutable(FGridPosition(4, 0))->Multiplier = 3.f;

    FConnectItBoardOperation_Shift Shift;
    Shift.Faction = 0;
    Shift.Direction = EGridDirection::Right;
    for (int32 X = 0; X < 5; X++)
    {
        Shift.Positions.Add(FGridPosition(X, 0));
    }

    // Which steps are allowed: 1 .. (shiftable tiles - 1), and anything that
    // isn't a whole number of laps
    FConnectItBoardOperation_Shift Zero = Shift;
    Zero.Steps = 0;
    TestFalse(TEXT("zero steps is refused"), Zero.CanApply(Board));

    FConnectItBoardOperation_Shift Negative = Shift;
    Negative.Steps = -2;
    TestFalse(TEXT("negative steps is refused"), Negative.CanApply(Board));

    FConnectItBoardOperation_Shift FullLap = Shift;
    FullLap.Steps = 5;
    TestFalse(TEXT("a full lap (steps = shiftable tiles) is refused"), FullLap.CanApply(Board));

    FConnectItBoardOperation_Shift LapAndOne = Shift;
    LapAndOne.Steps = 6;
    TestTrue(TEXT("a lap and one more is the same as one"), LapAndOne.CanApply(Board));

    // Two steps: every tile's data moves two places, the last two wrapping
    FConnectItBoardState Before = Board;
    Shift.Steps = 2;
    TestTrue(TEXT("two steps is valid"), Shift.CanApply(Board));

    FConnectItTouchedPositions Touched;
    FConnectItBoardChangeEvent Event;
    Shift.Apply(Board, Touched, &Event);

    TestEqual(TEXT("X=2 holds what X=0 held (faction 0)"), TileAt(Board, 2, 0).FactionPiece, 0);
    TestEqual(TEXT("X=3 holds what X=1 held (faction 1)"), TileAt(Board, 3, 0).FactionPiece, 1);
    TestEqual(TEXT("X=1 holds what X=4 held (multiplier 3, wrapped past the end)"),
        TileAt(Board, 1, 0).Multiplier, 3.f);
    TestEqual(TEXT("X=0 holds what X=3 held (empty, multiplier 1)"), TileAt(Board, 0, 0).Multiplier, 1.f);

    TestTrue(TEXT("both pieces are reported for scoring"),
        Touched.Num() == 2 && Touched.Contains(FGridPosition(2, 0)) && Touched.Contains(FGridPosition(3, 0)));

    const auto* Shifted = OnlyEvent<FConnectItBoardEvent_BoardShifted>(Event);
    if (!TestNotNull(TEXT("one Board Shifted event"), Shifted)) return false;
    TestEqual(TEXT("the event carries the step count"), Shifted->Steps, 2);
    if (!TestEqual(TEXT("one from/to pair per tile"), Shifted->StartPositions.Num(), 5)) return false;
    const int32 FromX0 = Shifted->StartPositions.IndexOfByKey(FGridPosition(0, 0));
    const int32 FromX4 = Shifted->StartPositions.IndexOfByKey(FGridPosition(4, 0));
    TestTrue(TEXT("X=0 -> X=2 is one of the pairs"),
        FromX0 != INDEX_NONE && Shifted->EndPositions[FromX0] == FGridPosition(2, 0));
    TestTrue(TEXT("X=4 -> X=1 (wrapped) is one of the pairs"),
        FromX4 != INDEX_NONE && Shifted->EndPositions[FromX4] == FGridPosition(1, 0));

    // Nothing is created or destroyed: same pieces, same multipliers, just moved
    int32 PiecesBefore = 0, PiecesAfter = 0;
    float MultiplierBefore = 0.f, MultiplierAfter = 0.f;
    for (int32 X = 0; X < 5; X++)
    {
        PiecesBefore += TileAt(Before, X, 0).FactionPiece != -1;
        PiecesAfter += TileAt(Board, X, 0).FactionPiece != -1;
        MultiplierBefore += TileAt(Before, X, 0).Multiplier;
        MultiplierAfter += TileAt(Board, X, 0).Multiplier;
    }
    TestEqual(TEXT("the same number of pieces"), PiecesAfter, PiecesBefore);
    TestEqual(TEXT("the same total multiplier"), MultiplierAfter, MultiplierBefore);

    // A fixed tile is jumped over, not counted: with X=2 fixed the shiftable
    // tiles are X = 0, 1, 3, 4, and two steps puts X=0's data on X=3
    FConnectItBoardState Pinned = Before;
    Pinned.GetTileDataMutable(FGridPosition(2, 0))->bCanShift = false;

    FConnectItBoardOperation_Shift PinnedShift = Shift;
    PinnedShift.Steps = 2;
    TestTrue(TEXT("a shift with a fixed tile in the line is valid"), PinnedShift.CanApply(Pinned));

    FConnectItTouchedPositions PinnedTouched;
    PinnedShift.Apply(Pinned, PinnedTouched, nullptr);
    TestEqual(TEXT("X=0's piece moved two shiftable places, over the fixed tile, to X=3"),
        TileAt(Pinned, 3, 0).FactionPiece, 0);
    TestEqual(TEXT("X=1's piece moved to X=4"), TileAt(Pinned, 4, 0).FactionPiece, 1);
    TestTrue(TEXT("the fixed tile kept its own data"),
        TileAt(Pinned, 2, 0).FactionPiece == -1 && !TileAt(Pinned, 2, 0).bCanShift);

    // With the fixed tile there are only 4 shiftable tiles, so 4 steps is a lap
    FConnectItBoardOperation_Shift PinnedLap = PinnedShift;
    PinnedLap.Steps = 4;
    TestFalse(TEXT("a lap of the shiftable tiles (4, not 5) is refused"), PinnedLap.CanApply(Pinned));
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
    const auto* Removed = OnlyEvent<FConnectItBoardEvent_PieceRemoved>(Event);
    TestTrue(TEXT("one Piece Removed event says whose piece it was"),
        Removed && Removed->Position == FGridPosition(1, 1) && Removed->RemovedFaction == 1);
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
    const auto* Destroyed = OnlyEvent<FConnectItBoardEvent_TileMultiplierDestroyed>(Event);
    TestTrue(TEXT("one Tile Multiplier Destroyed event says where"),
        Destroyed && Destroyed->Position == FGridPosition(1, 1));
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
    const auto* Toggled = OnlyEvent<FConnectItBoardEvent_TileActiveToggled>(Event);
    TestTrue(TEXT("one Tile Active Toggled event says where and the new state"),
        Toggled && Toggled->Position == FGridPosition(1, 1) && !Toggled->bNowActive);

    FConnectItBoardChangeEvent Again;
    Toggle.Apply(Board, Touched, &Again);
    const auto* ToggledBack = OnlyEvent<FConnectItBoardEvent_TileActiveToggled>(Again);
    TestTrue(TEXT("and back again"), TileAt(Board, 1, 1).bIsActive && ToggledBack && ToggledBack->bNowActive);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItBoardEventOrderTest,
    "ConnectIt.Board.Events.Order", TestFlags)

bool FConnectItBoardEventOrderTest::RunTest(const FString& Parameters)
{
    // The calls the Mediator makes for one request, on a placement that
    // completes a line: the change's events come out in the order things
    // happened, and say which category each belongs to.
    FConnectItBoardState Board = MakeBoard(7);
    Place(Board, 3, 0, 0);
    Place(Board, 3, 1, 0);
    Place(Board, 3, 2, 0);

    const FConnectItRuleSet Rules;
    FConnectItBoardChangeEvent Change;

    FConnectItTouchedPositions Touched;
    MakeAt<FConnectItBoardOperation_PlacePiece>(0, 3, 3).Apply(Board, Touched, &Change);
    Rules.ResolveBoardChange(Board, Touched, true, &Change);

    if (!TestEqual(TEXT("two events"), Change.Events.Num(), 2)) return false;

    TestNotNull(TEXT("first: the piece was placed"), EventAt<FConnectItBoardEvent_PiecePlaced>(Change, 0));
    TestNotNull(TEXT("...an operation event"), EventAt<FConnectItBoardOperationEvent>(Change, 0));
    TestNull(TEXT("...not a result event"), EventAt<FConnectItBoardResultEvent>(Change, 0));

    TestNotNull(TEXT("second: it scored"), EventAt<FConnectItBoardEvent_Scored>(Change, 1));
    TestNotNull(TEXT("...a result event"), EventAt<FConnectItBoardResultEvent>(Change, 1));
    TestNull(TEXT("...not an operation event"), EventAt<FConnectItBoardOperationEvent>(Change, 1));

    // Each event names the tag its listeners bind to
    const FConnectItBoardEvent* First = EventAt<FConnectItBoardEvent>(Change, 0);
    const FConnectItBoardEvent* Second = EventAt<FConnectItBoardEvent>(Change, 1);
    TestTrue(TEXT("tags: PiecePlaced then Scored"),
        First && Second
        && First->GetEventTag() == ConnectIt_Event_PiecePlaced
        && Second->GetEventTag() == ConnectIt_Event_Scored);

    TestNotNull(TEXT("FindFirst finds the scored event"), Change.FindFirst<FConnectItBoardEvent_Scored>());
    TestNull(TEXT("no win event without a win"), Change.FindFirst<FConnectItBoardEvent_GameWon>());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItLineDirectionsTest,
    "ConnectIt.Board.Rules.LineDirections", TestFlags)

bool FConnectItLineDirectionsTest::RunTest(const FString& Parameters)
{
    using EDir = EConnectItScoringDirection;

    // Points for faction 0 completing a line of four from (X, Y) stepping
    // (DX, DY), under a Lines rule that allows only Allowed
    const auto ScoreLine = [](int32 Allowed, int32 X, int32 Y, int32 DX, int32 DY)
    {
        FConnectItRuleSet Rules;
        Rules.ScoringRule.GetMutablePtr<FConnectItScoringRule_Lines>()->ScoringDirections = Allowed;

        FConnectItBoardState Board = MakeBoard(7);
        for (int32 Step = 0; Step < 4; Step++)
        {
            Place(Board, X + Step * DX, Y + Step * DY, 0);
        }
        // Scored as if the last piece of the line had just arrived
        return Rules.ApplyScoring(Board, FGridPosition(X + 3 * DX, Y + 3 * DY), 0, true);
    };

    // The rule's own axes: {0,1} is "horizontal" (a row), {1,0} "vertical"
    const int32 Horizontal = static_cast<int32>(EDir::Horizontal);
    const int32 Vertical = static_cast<int32>(EDir::Vertical);
    const int32 Diagonal = static_cast<int32>(EDir::Diagonal);
    const int32 All = Horizontal | Vertical | Diagonal;

    TestEqual(TEXT("default allows every direction"), FConnectItScoringRule_Lines().ScoringDirections, All);

    TestEqual(TEXT("horizontal only: a row scores"), ScoreLine(Horizontal, 1, 0, 0, 1), 4.f);
    TestEqual(TEXT("horizontal only: a column does not"), ScoreLine(Horizontal, 0, 1, 1, 0), 0.f);
    TestEqual(TEXT("horizontal only: a diagonal does not"), ScoreLine(Horizontal, 0, 0, 1, 1), 0.f);

    TestEqual(TEXT("vertical only: a column scores"), ScoreLine(Vertical, 0, 1, 1, 0), 4.f);
    TestEqual(TEXT("vertical only: a row does not"), ScoreLine(Vertical, 1, 0, 0, 1), 0.f);

    TestEqual(TEXT("diagonal only: one diagonal scores"), ScoreLine(Diagonal, 0, 0, 1, 1), 4.f);
    TestEqual(TEXT("diagonal only: the other diagonal scores"), ScoreLine(Diagonal, 0, 3, 1, -1), 4.f);
    TestEqual(TEXT("diagonal only: a row does not"), ScoreLine(Diagonal, 1, 0, 0, 1), 0.f);

    TestEqual(TEXT("horizontal + vertical: a row scores"), ScoreLine(Horizontal | Vertical, 1, 0, 0, 1), 4.f);
    TestEqual(TEXT("horizontal + vertical: a diagonal does not"), ScoreLine(Horizontal | Vertical, 0, 0, 1, 1), 0.f);

    TestEqual(TEXT("nothing allowed: nothing scores"), ScoreLine(0, 1, 0, 0, 1), 0.f);

    // The AI only values lines-in-the-making along directions that can score
    const FConnectItBoardState Board = MakeBoard(7);
    const auto WindowCount = [&Board](int32 Allowed)
    {
        FConnectItRuleSet Rules = RulesWonAt(100.f);
        Rules.ScoringRule.GetMutablePtr<FConnectItScoringRule_Lines>()->ScoringDirections = Allowed;
        const FConnectItMinMaxRules MinMaxRules(Board, Rules,
            DefaultEvaluationWeights(), DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());
        return MinMaxRules.GetGeometry().LineWindows.Num();
    };
    // 7x7, lines of 4: 4 windows per row or column (x7), 4x4 per diagonal direction
    TestEqual(TEXT("all directions: 88 line windows"), WindowCount(All), 28 + 28 + 16 + 16);
    TestEqual(TEXT("horizontal only: 28"), WindowCount(Horizontal), 28);
    TestEqual(TEXT("diagonal only: 32"), WindowCount(Diagonal), 32);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItSimulateOperationTest,
    "ConnectIt.Board.SimulateOperation", TestFlags)

bool FConnectItSimulateOperationTest::RunTest(const FString& Parameters)
{
    // What a Blueprint AI strategy calls to look before it moves
    using FLibrary = UConnectIt_BoardStateLibrary;

    // A row and a column of three for faction 0, both missing (3,3); a
    // faction 1 piece and an inactive tile elsewhere
    FConnectItBoardState Board = MakeBoard(7);
    Place(Board, 3, 0, 0); Place(Board, 3, 1, 0); Place(Board, 3, 2, 0);
    Place(Board, 0, 3, 0); Place(Board, 1, 3, 0); Place(Board, 2, 3, 0);
    Place(Board, 6, 6, 1);
    Board.GetTileDataMutable(FGridPosition(5, 5))->bIsActive = false;

    const FConnectItRuleSet Rules;

    const TArray<FGridPosition> Placeable = FLibrary::GetPlaceablePositions(Board);
    TestEqual(TEXT("placeable = every tile but the 7 occupied and 1 inactive"), Placeable.Num(), 49 - 8);
    TestFalse(TEXT("...not an occupied one"), Placeable.Contains(FGridPosition(6, 6)));
    TestFalse(TEXT("...not an inactive one"), Placeable.Contains(FGridPosition(5, 5)));

    const auto Simulate = [&](int32 Faction, int32 X, int32 Y,
        FConnectItBoardState& OutBoard, float& OutPoints, int32& OutNumScored)
    {
        // The operation's own Faction is deliberately left unset
        FConnectItBoardOperation_PlacePiece Operation;
        Operation.Position = FGridPosition(X, Y);
        FConnectItBoardChangeEvent Events;
        return FLibrary::SimulateOperation(Board, Rules, FInstancedStruct::Make(Operation), Faction,
            OutBoard, OutPoints, OutNumScored, Events);
    };

    FConnectItBoardState Result;
    float Points = 0.f;
    int32 NumScored = 0;

    // A quiet move
    TestTrue(TEXT("an empty tile can be simulated"), Simulate(0, 6, 0, Result, Points, NumScored));
    TestEqual(TEXT("...it scores nothing"), NumScored, 0);
    TestEqual(TEXT("...the piece is on the result board, for the faction given"), TileAt(Result, 6, 0).FactionPiece, 0);
    TestEqual(TEXT("...and not on the real one"), TileAt(Board, 6, 0).FactionPiece, -1);

    // The crossing point completes two lines at once
    TestTrue(TEXT("the crossing point can be simulated"), Simulate(0, 3, 3, Result, Points, NumScored));
    TestEqual(TEXT("...it completes two lines"), NumScored, 2);
    TestTrue(TEXT("...and scores"), Points > 0.f);
    TestEqual(TEXT("...the result board has the score"), Result.GetScore(0), Points);
    TestEqual(TEXT("...the real board does not"), Board.GetScore(0), 0.f);
    TestEqual(TEXT("...and still holds the line"), TileAt(Board, 3, 0).FactionPiece, 0);

    // The same tile for the other faction completes nothing
    TestTrue(TEXT("the same tile for faction 1"), Simulate(1, 3, 3, Result, Points, NumScored));
    TestEqual(TEXT("...scores nothing"), NumScored, 0);

    // Moves that can't be made
    TestFalse(TEXT("an occupied tile takes the False pin"), Simulate(0, 6, 6, Result, Points, NumScored));
    TestEqual(TEXT("...and the result is just the board"), Result.GetScore(0), 0.f);
    TestFalse(TEXT("so does an inactive tile"), Simulate(0, 5, 5, Result, Points, NumScored));

    FConnectItBoardChangeEvent Events;
    TestFalse(TEXT("so does a struct that isn't a board operation"),
        FLibrary::SimulateOperation(Board, Rules, FInstancedStruct::Make(FGridPosition(0, 0)), 0,
            Result, Points, NumScored, Events));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAICandidatesTest,
    "ConnectIt.AI.Candidates", TestFlags)

bool FConnectItAICandidatesTest::RunTest(const FString& Parameters)
{
    // A strategy's decision is capped at MaxCandidates, best (first) kept
    UConnectIt_TestAIStrategy_FixedCandidates* Strategy = NewObject<UConnectIt_TestAIStrategy_FixedCandidates>();
    Strategy->NumCandidates = 8;
    Strategy->MaxCandidates = 3;

    Strategy->StartDecision(FConnectItAIDecisionContext());
    TestTrue(TEXT("answered at once"), Strategy->IsDecisionFinished());
    const FConnectItAIDecision Received = Strategy->GetBestSoFar();
    if (TestEqual(TEXT("trimmed to MaxCandidates"), Received.Candidates.Num(), 3))
    {
        TestEqual(TEXT("best first"), Received.Candidates[0].Summary, FString(TEXT("0")));
        TestEqual(TEXT("order kept"), Received.Candidates[2].Summary, FString(TEXT("2")));
    }

    Strategy->NumCandidates = 2;
    Strategy->StartDecision(FConnectItAIDecisionContext());
    TestEqual(TEXT("fewer than the cap are all kept"), Strategy->GetBestSoFar().Candidates.Num(), 2);

    // The single-move helper
    FConnectItBoardOperation_PlacePiece Place;
    Place.Position = FGridPosition(1, 1);
    const FConnectItAIDecision Single = UConnectIt_AIStrategy::MakeSingleMoveDecision(FInstancedStruct::Make(Place), TEXT("why"));
    TestTrue(TEXT("one-move decision has its move"), Single.HasMove() && Single.Candidates.Num() == 1);
    TestFalse(TEXT("no operation, no move"), UConnectIt_AIStrategy::MakeSingleMoveDecision(FInstancedStruct(), TEXT("")).HasMove());

    // What the AI may do now
    FConnectItAIDecisionContext Context;
    Context.AvailableRequestTypes.AddTag(ConnectIt_Game_PlacePiece);
    TestTrue(TEXT("a place move is available"),
        UConnectIt_AIStrategy::IsOperationAvailable(Context, FInstancedStruct::Make(Place)));
    TestFalse(TEXT("a swap is not"),
        UConnectIt_AIStrategy::IsOperationAvailable(Context, FInstancedStruct::Make(FConnectItBoardOperation_SwapPieces())));

    // Every kind of board operation is found
    const TArray<FGameplayTag>& AllTypes = FConnectItBoardOperation::GetAllRequestTypes();
    TestTrue(TEXT("all request types include place, swap and shift"),
        AllTypes.Contains(FGameplayTag(ConnectIt_Game_PlacePiece))
        && AllTypes.Contains(FGameplayTag(ConnectIt_Game_SwapPieces))
        && AllTypes.Contains(FGameplayTag(ConnectIt_Game_Shift)));
    TestEqual(TEXT("one per kind of operation"), AllTypes.Num(), 8);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMinMaxSearchToolTest,
    "ConnectIt.AI.MinMax.SearchTool", TestFlags)

bool FConnectItMinMaxSearchToolTest::RunTest(const FString& Parameters)
{
    // The search as the Blueprint node and the MinMax strategy run it --
    // prepared from a decision context and settings, run, converted --
    // done synchronously here.
    FConnectItBoardState Board = MakeBoard(7);
    Place(Board, 3, 0, 0);
    Place(Board, 3, 1, 0);
    Place(Board, 3, 2, 0);

    FConnectItAIDecisionContext Context;
    Context.Board = Board;
    Context.OwnSlot = 0;
    Context.Rules = RulesWonAt(4.f);
    Context.AvailableRequestTypes.AddTag(ConnectIt_Game_PlacePiece);

    FConnectItMinMaxSearchSettings Settings;
    Settings.MaxDepth = 1;
    Settings.DefaultTimeBudgetSeconds = 0.f;
    Settings.OpponentRequestTypes = PlaceOnly();

    ConnectItMinMaxSearch::FPrepared Prepared;
    FString WhyNot;
    if (!TestTrue(TEXT("a two-faction board with place moves can be searched"),
        ConnectItMinMaxSearch::Prepare(Context, Settings, Prepared, &WhyNot))) return false;

    const FConnectItMinMaxSearchResult Result =
        ConnectItMinMaxSearch::ToResult(FConnectItMinMaxSearch::Run(*Prepared.Rules, Prepared.Root, Prepared.Params));

    if (!TestTrue(TEXT("every place move is ranked"), Result.RankedMoves.Num() == 49 - 3)) return false;
    const FConnectItBoardOperation_PlacePiece* Best =
        Result.RankedMoves[0].Operation.GetPtr<FConnectItBoardOperation_PlacePiece>();
    TestTrue(TEXT("the best move takes the immediate win at (3,3)"), Best && Best->Position == FGridPosition(3, 3));
    TestTrue(TEXT("ranked best first"), Result.RankedMoves[0].Score >= Result.RankedMoves.Last().Score);

    const FConnectItAIDecision Decision = UConnectIt_FindBestMovesAsync::MakeDecisionFromSearch(Result, 3);
    TestEqual(TEXT("a decision from the search keeps the top 3"), Decision.Candidates.Num(), 3);

    // Explicit move sets replace the context's -- and the search only models place moves
    FConnectItMinMaxSearchSettings SwapOnly = Settings;
    SwapOnly.OwnRequestTypes.AddTag(ConnectIt_Game_SwapPieces);
    TestFalse(TEXT("a side with only swaps can't be searched"),
        ConnectItMinMaxSearch::Prepare(Context, SwapOnly, Prepared, &WhyNot));

    FConnectItAIDecisionContext NothingAvailable = Context;
    NothingAvailable.AvailableRequestTypes.Reset();
    TestFalse(TEXT("nothing available, nothing to search"),
        ConnectItMinMaxSearch::Prepare(NothingAvailable, Settings, Prepared, &WhyNot));

    FConnectItAIDecisionContext ThreeFactions = Context;
    ThreeFactions.Board.ScoreBoard = { 0.f, 0.f, 0.f };
    TestFalse(TEXT("only two factions"), ConnectItMinMaxSearch::Prepare(ThreeFactions, Settings, Prepared, &WhyNot));

    // Time: the decision's budget when it has one, else the settings' default
    FConnectItMinMaxSearchSettings Timed = Settings;
    Timed.DefaultTimeBudgetSeconds = 1.5f;
    if (ConnectItMinMaxSearch::Prepare(Context, Timed, Prepared, &WhyNot))
    {
        TestEqual(TEXT("no decision budget: the default"), Prepared.Params.TimeBudgetSeconds, 1.5);
    }
    FConnectItAIDecisionContext Budgeted = Context;
    Budgeted.TimeBudgetSeconds = 3.f;
    if (ConnectItMinMaxSearch::Prepare(Budgeted, Timed, Prepared, &WhyNot))
    {
        TestEqual(TEXT("a decision budget wins, even over a smaller default"), Prepared.Params.TimeBudgetSeconds, 3.0);
    }

    TestTrue(TEXT("the MinMax strategy declares place moves"),
        GetDefault<UConnectIt_AIStrategy_MinMax>()->GetSupportedRequestTypes().HasTagExact(ConnectIt_Game_PlacePiece));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMinMaxDepthProgressTest,
    "ConnectIt.AI.MinMax.DepthProgress", TestFlags)

bool FConnectItMinMaxDepthProgressTest::RunTest(const FString& Parameters)
{
    // The search reports each completed depth's ranking as it goes; the last
    // report is exactly the final result
    FConnectItBoardState Board = MakeBoard(5);
    Place(Board, 1, 1, 0);
    Place(Board, 3, 3, 1);

    const FConnectItMinMaxRules Rules(Board, RulesWonAt(100.f), DefaultEvaluationWeights(), DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());
    const auto Root = Rules.MakeRoot(Board, 0);

    TArray<int32> DepthsReported;
    FConnectItMinMaxSearchProgress::FScoredMoves LastReported;
    const auto Result = FConnectItMinMaxSearch::Run(Rules, Root, Depth(3),
        [&](const FConnectItMinMaxSearchProgress::FScoredMoves& RootScores, int32 ReportedDepth)
        {
            DepthsReported.Add(ReportedDepth);
            LastReported = RootScores;
        });

    TestTrue(TEXT("one report per depth, in order"), DepthsReported == TArray<int32>({ 1, 2, 3 }));
    if (!TestEqual(TEXT("the last report ranks every move"), LastReported.Num(), Result.RootScores.Num())) return false;
    for (int32 Index = 0; Index < LastReported.Num(); Index++)
    {
        TestTrue(FString::Printf(TEXT("rank %d matches the result"), Index),
            PlacePosition(LastReported[Index].Move) == PlacePosition(Result.RootScores[Index].Move)
            && LastReported[Index].Score == Result.RootScores[Index].Score);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMinMaxSearchProgressTest,
    "ConnectIt.AI.MinMax.SearchProgress", TestFlags)

bool FConnectItMinMaxSearchProgressTest::RunTest(const FString& Parameters)
{
    // A progress holder keeps the deepest completed ranking even when the
    // search is then stopped -- the case a strategy asked early relies on
    FConnectItBoardState Board = MakeBoard(7);
    Place(Board, 3, 0, 0);
    Place(Board, 3, 1, 0);
    Place(Board, 3, 2, 0);

    FConnectItAIDecisionContext Context;
    Context.Board = Board;
    Context.OwnSlot = 0;
    Context.Rules = RulesWonAt(4.f);
    Context.AvailableRequestTypes.AddTag(ConnectIt_Game_PlacePiece);

    FConnectItMinMaxSearchSettings Settings;
    Settings.MaxDepth = 3;
    Settings.DefaultTimeBudgetSeconds = 0.f;
    Settings.OpponentRequestTypes = PlaceOnly();

    ConnectItMinMaxSearch::FPrepared Prepared;
    if (!TestTrue(TEXT("prepared"), ConnectItMinMaxSearch::Prepare(Context, Settings, Prepared))) return false;

    FConnectItMinMaxSearchProgress Progress;
    TestEqual(TEXT("nothing published before depth 1"), Progress.GetRankedMoves().Num(), 0);

    // Stop the search as soon as depth 1 is reported
    std::atomic<bool> Cancelled(false);
    Prepared.Params.CancelFlag = &Cancelled;
    const auto Raw = FConnectItMinMaxSearch::Run(*Prepared.Rules, Prepared.Root, Prepared.Params,
        [&](const FConnectItMinMaxSearchProgress::FScoredMoves& RootScores, int32 ReportedDepth)
        {
            Progress.Publish(RootScores, ReportedDepth);
            Cancelled = true;
        });

    TestTrue(TEXT("the search itself was cancelled"), Raw.bCancelled);
    TestEqual(TEXT("a cancelled search returns nothing"), Raw.RootScores.Num(), 0);

    int32 DepthReached = 0;
    const TArray<FConnectItMinMaxRankedMove> Ranked = Progress.GetRankedMoves(&DepthReached);
    TestEqual(TEXT("the holder kept depth 1"), DepthReached, 1);
    TestEqual(TEXT("depth reached getter agrees"), Progress.GetDepthReached(), 1);
    if (!TestEqual(TEXT("the holder ranks every move"), Ranked.Num(), 49 - 3)) return false;
    const FConnectItBoardOperation_PlacePiece* Best = Ranked[0].Operation.GetPtr<FConnectItBoardOperation_PlacePiece>();
    TestTrue(TEXT("its best move is the immediate win at (3,3)"), Best && Best->Position == FGridPosition(3, 3));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAIBestSoFarTest,
    "ConnectIt.AI.BestSoFar", TestFlags)

bool FConnectItAIBestSoFarTest::RunTest(const FString& Parameters)
{
    const auto MakeDecision = [](int32 NumCandidates)
    {
        FConnectItAIDecision Decision;
        for (int32 Index = 0; Index < NumCandidates; Index++)
        {
            FConnectItBoardOperation_PlacePiece Operation;
            Operation.Position = FGridPosition(0, Index);
            Decision.Candidates.Add({ FInstancedStruct::Make(Operation), FString::FromInt(Index) });
        }
        return Decision;
    };

    UConnectIt_TestAIStrategy_Silent* Strategy = NewObject<UConnectIt_TestAIStrategy_Silent>();
    Strategy->MaxCandidates = 2;

    Strategy->StartDecision(FConnectItAIDecisionContext());
    TestFalse(TEXT("nothing so far"), Strategy->GetBestSoFar().HasMove());

    Strategy->Publish(MakeDecision(4));
    TestEqual(TEXT("a published answer is the best so far, trimmed"), Strategy->GetBestSoFar().Candidates.Num(), 2);
    TestFalse(TEXT("publishing isn't finishing"), Strategy->IsDecisionFinished());
    TestEqual(TEXT("candidates are labelled with the class name"),
        Strategy->GetBestSoFar().Candidates[0].SourceLabel, FName(TEXT("ConnectIt_TestAIStrategy_Silent")));

    Strategy->StopDecision();
    TestTrue(TEXT("the published answer outlives the cancel"), Strategy->GetBestSoFar().HasMove());
    Strategy->Publish(MakeDecision(1));
    TestEqual(TEXT("publishing after a cancel is ignored"), Strategy->GetBestSoFar().Candidates.Num(), 2);

    Strategy->Label = TEXT("Slow");
    Strategy->StartDecision(FConnectItAIDecisionContext());
    TestFalse(TEXT("a new decision clears the old answer"), Strategy->GetBestSoFar().HasMove());
    Strategy->AnswerLate(MakeDecision(1));
    TestTrue(TEXT("finishing marks it finished"), Strategy->IsDecisionFinished());
    TestEqual(TEXT("the final answer is the best so far"), Strategy->GetBestSoFar().Candidates.Num(), 1);
    TestEqual(TEXT("an explicit label wins"), Strategy->GetBestSoFar().Candidates[0].SourceLabel, FName(TEXT("Slow")));

    // A label the strategy set itself is kept
    FConnectItAIDecision Labelled = MakeDecision(1);
    Labelled.Candidates[0].SourceLabel = TEXT("Mine");
    Strategy->StartDecision(FConnectItAIDecisionContext());
    Strategy->AnswerLate(Labelled);
    TestEqual(TEXT("a candidate's own label is kept"), Strategy->GetBestSoFar().Candidates[0].SourceLabel, FName(TEXT("Mine")));

    TestFalse(TEXT("no confidence by default"), UConnectIt_AIStrategy::CandidateHasConfidence(FConnectItAICandidate()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMinMaxConfidenceTest,
    "ConnectIt.AI.MinMax.Confidence", TestFlags)

bool FConnectItMinMaxConfidenceTest::RunTest(const FString& Parameters)
{
    const auto Ranked = [](std::initializer_list<int32> Scores)
    {
        TArray<FConnectItMinMaxRankedMove> Moves;
        for (const int32 Score : Scores)
        {
            FConnectItMinMaxRankedMove& Move = Moves.AddDefaulted_GetRef();
            Move.Operation = FInstancedStruct::Make(FConnectItBoardOperation_PlacePiece());
            Move.Score = Score;
        }
        return Moves;
    };

    const int32 Win = FConnectItMinMaxRules::WinValue - 1;
    const TArray<float> Mixed = ConnectItMinMaxSearch::ComputeConfidences(Ranked({ Win, 30, 20, 10, -Win }));
    if (!TestEqual(TEXT("one per move"), Mixed.Num(), 5)) return false;
    TestEqual(TEXT("a forced win is certain"), Mixed[0], 1.f);
    TestEqual(TEXT("the best ordinary move is 1"), Mixed[1], 1.f);
    TestEqual(TEXT("the middle one is halfway"), Mixed[2], 0.5f);
    TestEqual(TEXT("the worst ordinary move is 0"), Mixed[3], 0.f);
    TestEqual(TEXT("a forced loss is 0"), Mixed[4], 0.f);

    TestTrue(TEXT("a single move is 1"), ConnectItMinMaxSearch::ComputeConfidences(Ranked({ 7 })) == TArray<float>({ 1.f }));
    TestTrue(TEXT("all equal are 1"), ConnectItMinMaxSearch::ComputeConfidences(Ranked({ 5, 5, 5 })) == TArray<float>({ 1.f, 1.f, 1.f }));

    const FConnectItAIDecision Decision = ConnectItMinMaxSearch::MakeDecision(Ranked({ 30, 20, 10 }), TEXT("test"), 2);
    if (!TestEqual(TEXT("capped at MaxCandidates"), Decision.Candidates.Num(), 2)) return false;
    TestEqual(TEXT("raw score kept"), Decision.Candidates[1].StrategyScore, 20.f);
    TestEqual(TEXT("confidence across the whole ranking"), Decision.Candidates[1].Confidence, 0.5f);
    TestTrue(TEXT("it has a confidence"), UConnectIt_AIStrategy::CandidateHasConfidence(Decision.Candidates[0]));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMinMaxStrategyPartialTest,
    "ConnectIt.AI.MinMax.StrategyPartialAnswer", TestFlags)

bool FConnectItMinMaxStrategyPartialTest::RunTest(const FString& Parameters)
{
    // A MinMax strategy asked before its search finishes answers with the
    // deepest ranking completed so far. The search runs on a background task,
    // so wait (bounded) for depth 1 to arrive.
    FConnectItBoardState Board = MakeBoard(7);
    Place(Board, 3, 0, 0);
    Place(Board, 3, 1, 0);
    Place(Board, 3, 2, 0);

    FConnectItAIDecisionContext Context;
    Context.Board = Board;
    Context.OwnSlot = 0;
    Context.Rules = RulesWonAt(4.f);
    Context.AvailableRequestTypes.AddTag(ConnectIt_Game_PlacePiece);

    UConnectIt_AIStrategy_MinMax* Strategy = NewObject<UConnectIt_AIStrategy_MinMax>();
    Strategy->Search.MaxDepth = 10;
    Strategy->Search.DefaultTimeBudgetSeconds = 0.f;
    Strategy->Search.OpponentRequestTypes = PlaceOnly();
    Strategy->MaxCandidates = 3;

    Strategy->StartDecision(Context);

    FConnectItAIDecision Partial;
    const double GiveUpAt = FPlatformTime::Seconds() + 10.0;
    while (!Partial.HasMove() && FPlatformTime::Seconds() < GiveUpAt)
    {
        FPlatformProcess::Sleep(0.005f);
        Partial = Strategy->GetBestSoFar();
    }
    TestFalse(TEXT("a depth-10 search on an open 7x7 board hasn't finished yet"), Strategy->IsDecisionFinished());
    Strategy->StopDecision();

    if (!TestEqual(TEXT("a partial answer, trimmed"), Partial.Candidates.Num(), 3)) return false;
    const FConnectItBoardOperation_PlacePiece* Best = Partial.Candidates[0].Payload.GetPtr<FConnectItBoardOperation_PlacePiece>();
    TestTrue(TEXT("its best move is already the immediate win at (3,3)"), Best && Best->Position == FGridPosition(3, 3));
    TestEqual(TEXT("a forced win is certain"), Partial.Candidates[0].Confidence, 1.f);
    TestEqual(TEXT("labelled as the MinMax strategy"),
        Partial.Candidates[0].SourceLabel, FName(TEXT("ConnectIt_AIStrategy_MinMax")));
    TestTrue(TEXT("the summary says the search wasn't finished"),
        Partial.Candidates[0].Summary.Contains(TEXT("not finished")));
    return true;
}

namespace ConnectItMinMaxTests
{
    FConnectItAICandidate PlaceCandidate(int32 X, int32 Y, float Confidence = -1.f)
    {
        FConnectItBoardOperation_PlacePiece Operation;
        Operation.Position = FGridPosition(X, Y);
        FConnectItAICandidate Candidate;
        Candidate.Payload = FInstancedStruct::Make(Operation);
        Candidate.Summary = FString::Printf(TEXT("%d,%d"), X, Y);
        Candidate.Confidence = Confidence;
        return Candidate;
    }

    FConnectItAIStrategyAnswer Answer(int32 StrategyIndex, const TArray<FConnectItAICandidate>& Candidates)
    {
        FConnectItAIStrategyAnswer Result;
        Result.StrategyIndex = StrategyIndex;
        Result.bFinished = true;
        Result.Decision.Candidates = Candidates;
        return Result;
    }

    // "X,Y" of a place candidate, comparable with TestEqual
    FString CandidatePosition(const FConnectItAICandidate& Candidate)
    {
        const FConnectItBoardOperation_PlacePiece* Place = Candidate.Payload.GetPtr<FConnectItBoardOperation_PlacePiece>();
        return Place ? FString::Printf(TEXT("%d,%d"), Place->Position.X, Place->Position.Y) : FString(TEXT("none"));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAISelectorEndorseTest,
    "ConnectIt.AI.Selector.Endorse", TestFlags)

bool FConnectItAISelectorEndorseTest::RunTest(const FString& Parameters)
{
    // Given out of order: strategy 1 then strategy 0
    const TArray<FConnectItAIStrategyAnswer> Answers = {
        Answer(1, { PlaceCandidate(5, 5), PlaceCandidate(1, 1) }),
        Answer(0, { PlaceCandidate(1, 1), PlaceCandidate(2, 2), PlaceCandidate(2, 2) }),
    };

    const TArray<FConnectItAIEndorsedCandidate> Endorsed = UConnectIt_AISelector::Endorse(Answers);
    if (!TestEqual(TEXT("three distinct moves"), Endorsed.Num(), 3)) return false;
    TestEqual(TEXT("priority order: strategy 0's first move first"), CandidatePosition(Endorsed[0].Candidate), TEXT("1,1"));
    TestEqual(TEXT("proposed by both strategies"), Endorsed[0].Endorsements, 2);
    TestEqual(TEXT("kept as the higher-priority strategy gave it"), Endorsed[0].FirstStrategyIndex, 0);
    TestEqual(TEXT("then strategy 0's second"), CandidatePosition(Endorsed[1].Candidate), TEXT("2,2"));
    TestEqual(TEXT("listed twice by one strategy is one endorsement"), Endorsed[1].Endorsements, 1);
    TestEqual(TEXT("then strategy 1's own"), CandidatePosition(Endorsed[2].Candidate), TEXT("5,5"));
    TestEqual(TEXT("from strategy 1"), Endorsed[2].FirstStrategyIndex, 1);

    // First Available: that order, trimmed
    UConnectIt_AISelector_FirstAvailable* First = NewObject<UConnectIt_AISelector_FirstAvailable>();
    First->MaxCandidates = 2;
    const FConnectItAIDecision Decision = First->Select(FConnectItAIDecisionContext(), Answers);
    if (!TestEqual(TEXT("trimmed to MaxCandidates"), Decision.Candidates.Num(), 2)) return false;
    TestEqual(TEXT("first available is the top-priority move"), CandidatePosition(Decision.Candidates[0]), TEXT("1,1"));
    TestFalse(TEXT("no answers, no move"), First->Select(FConnectItAIDecisionContext(), {}).HasMove());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAISelectorTopNTest,
    "ConnectIt.AI.Selector.TopN", TestFlags)

bool FConnectItAISelectorTopNTest::RunTest(const FString& Parameters)
{
    const TArray<FConnectItAIStrategyAnswer> Answers = {
        Answer(0, { PlaceCandidate(0, 0), PlaceCandidate(0, 1), PlaceCandidate(0, 2), PlaceCandidate(0, 3) }) };

    UConnectIt_AISelector_TopN* Selector = NewObject<UConnectIt_AISelector_TopN>();
    Selector->TopMovesConsidered = 3;

    Selector->MistakeChance = 0.f;
    TestEqual(TEXT("no mistakes: the best move"),
        CandidatePosition(Selector->Select(FConnectItAIDecisionContext(), Answers).Candidates[0]), TEXT("0,0"));

    // Always a mistake: one of ranks 2..3, never the best, never rank 4; both seen
    Selector->MistakeChance = 1.f;
    bool bSawRank2 = false, bSawRank3 = false;
    for (int32 Seed = 1; Seed <= 50; Seed++)
    {
        Selector->RandomSeed = Seed;
        const FConnectItAIDecision Decision = Selector->Select(FConnectItAIDecisionContext(), Answers);
        const FString Picked = CandidatePosition(Decision.Candidates[0]);
        TestTrue(TEXT("a mistake is rank 2 or 3"), Picked == TEXT("0,1") || Picked == TEXT("0,2"));
        bSawRank2 |= Picked == TEXT("0,1");
        bSawRank3 |= Picked == TEXT("0,2");
        TestEqual(TEXT("every move is still offered as a fallback"), Decision.Candidates.Num(), 4);
    }
    TestTrue(TEXT("both mistakes happen"), bSawRank2 && bSawRank3);

    Selector->RandomSeed = 7;
    const FString FirstRun = CandidatePosition(Selector->Select(FConnectItAIDecisionContext(), Answers).Candidates[0]);
    TestEqual(TEXT("a fixed seed repeats"),
        CandidatePosition(Selector->Select(FConnectItAIDecisionContext(), Answers).Candidates[0]), FirstRun);

    Selector->TopMovesConsidered = 1;
    TestEqual(TEXT("top 1: no room for a mistake"),
        CandidatePosition(Selector->Select(FConnectItAIDecisionContext(), Answers).Candidates[0]), TEXT("0,0"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAISelectorGreedyTest,
    "ConnectIt.AI.Selector.GreedyScorer", TestFlags)

bool FConnectItAISelectorGreedyTest::RunTest(const FString& Parameters)
{
    // Faction 0 completes a 4-line at (3,3); (0,6) scores nothing; (3,0) is occupied
    FConnectItBoardState Board = MakeBoard(7);
    Place(Board, 3, 0, 0);
    Place(Board, 3, 1, 0);
    Place(Board, 3, 2, 0);

    FConnectItAIDecisionContext Context;
    Context.Board = Board;
    Context.OwnSlot = 0;
    Context.Rules = RulesWonAt(100.f);

    const TArray<FConnectItAIStrategyAnswer> Answers = {
        Answer(0, { PlaceCandidate(3, 0), PlaceCandidate(0, 6), PlaceCandidate(3, 3), PlaceCandidate(6, 6) }) };

    UConnectIt_AISelector_GreedyScorer* Selector = NewObject<UConnectIt_AISelector_GreedyScorer>();
    const FConnectItAIDecision Decision = Selector->Select(Context, Answers);
    if (!TestEqual(TEXT("every candidate ranked"), Decision.Candidates.Num(), 4)) return false;
    TestEqual(TEXT("the scoring move first"), CandidatePosition(Decision.Candidates[0]), TEXT("3,3"));
    TestEqual(TEXT("equal gains keep priority order"), CandidatePosition(Decision.Candidates[1]), TEXT("0,6"));
    TestEqual(TEXT("then the next"), CandidatePosition(Decision.Candidates[2]), TEXT("6,6"));
    TestEqual(TEXT("an impossible move last"), CandidatePosition(Decision.Candidates[3]), TEXT("3,0"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAISelectorWeightedTest,
    "ConnectIt.AI.Selector.WeightedByConfidence", TestFlags)

bool FConnectItAISelectorWeightedTest::RunTest(const FString& Parameters)
{
    const TArray<FConnectItAIStrategyAnswer> Answers = {
        Answer(0, { PlaceCandidate(0, 0, 0.1f), PlaceCandidate(0, 1, 0.9f), PlaceCandidate(0, 2) }) };

    UConnectIt_AISelector_WeightedByConfidence* Selector = NewObject<UConnectIt_AISelector_WeightedByConfidence>();

    // Cold: the most confident move every time
    Selector->Temperature = 0.01f;
    for (int32 Seed = 1; Seed <= 30; Seed++)
    {
        Selector->RandomSeed = Seed;
        const FConnectItAIDecision Decision = Selector->Select(FConnectItAIDecisionContext(), Answers);
        TestEqual(TEXT("cold picks the most confident"), CandidatePosition(Decision.Candidates[0]), TEXT("0,1"));
        TestEqual(TEXT("the rest stay as fallbacks"), Decision.Candidates.Num(), 3);
    }

    // A missing confidence counts as MissingConfidence
    Selector->MissingConfidence = 1.f;
    Selector->RandomSeed = 3;
    TestEqual(TEXT("missing confidence set high wins when cold"),
        CandidatePosition(Selector->Select(FConnectItAIDecisionContext(), Answers).Candidates[0]), TEXT("0,2"));

    // Hot: every move gets picked sometimes
    Selector->MissingConfidence = 0.5f;
    Selector->Temperature = 10.f;
    TSet<FString> Seen;
    for (int32 Seed = 1; Seed <= 200; Seed++)
    {
        Selector->RandomSeed = Seed;
        Seen.Add(CandidatePosition(Selector->Select(FConnectItAIDecisionContext(), Answers).Candidates[0]));
    }
    TestEqual(TEXT("hot picks every move at some point"), Seen.Num(), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMatchConditionAssetsTest,
    "ConnectIt.Board.Conditions.Assets", TestFlags)

bool FConnectItMatchConditionAssetsTest::RunTest(const FString& Parameters)
{
    // Goals as assets: the rule set copies their conditions out at match
    // start, in order, and a checkpoint names the asset reached
    UConnectIt_MatchConditionAsset* ScoreGoal = NewObject<UConnectIt_MatchConditionAsset>(
        GetTransientPackage(), TEXT("WC_Test_Score4"));
    ScoreGoal->Conditions.Add(TInstancedStruct<FConnectItMatchCondition>::Make<FConnectItMatchCondition_ScoreThreshold>(ScoreAtLeast(4.f)));
    UConnectIt_MatchConditionAsset* ClearGoal = NewObject<UConnectIt_MatchConditionAsset>(
        GetTransientPackage(), TEXT("WC_Test_Clear"));
    ClearGoal->Conditions.Add(TInstancedStruct<FConnectItMatchCondition>::Make<FConnectItMatchCondition_PieceCount>(PiecesIs(EConnectItPieceCountComparison::Equal, 0)));

    FConnectItRuleSet Rules;
    Rules.PlayerWinConditions = { ScoreGoal, nullptr, ClearGoal };
    Rules.AIWinConditions = { ScoreGoal };
    TestEqual(TEXT("nothing to play with before resolving"), Rules.PlayerWinGroups.Num(), 0);

    AddExpectedError(TEXT("Player win condition 1 is empty"), EAutomationExpectedErrorFlags::Contains, 1);
    Rules.ResolveWinGroups();
    if (!TestEqual(TEXT("one group per goal, the empty one skipped"), Rules.PlayerWinGroups.Num(), 2)) return false;
    TestTrue(TEXT("in order, with their source"),
        Rules.PlayerWinGroups[0].Source == ScoreGoal && Rules.PlayerWinGroups[1].Source == ClearGoal);
    TestEqual(TEXT("named after the asset"), Rules.PlayerWinGroups[1].Name, FName(TEXT("WC_Test_Clear")));
    TestEqual(TEXT("the conditions copied"), Rules.PlayerWinGroups[0].Conditions.Num(), 1);
    TestEqual(TEXT("the AI's too"), Rules.AIWinGroups.Num(), 1);

    // A copy: changing the match's target leaves the asset alone
    TestTrue(TEXT("a target can be set"), Rules.SetTargetScore(9.f));
    float AssetTarget = 0.f;
    TestTrue(TEXT("the asset still says 4"),
        ScoreGoal->Conditions[0].GetPtr()->GetTargetScore(AssetTarget) && AssetTarget == 4.f);

    // Reaching the first goal is a checkpoint naming its asset
    FConnectItBoardState Board = MakeBoard(5);
    Place(Board, 0, 0, 0);
    Board.StartingPieces = { 1, 1 };
    Board.ScoreBoard = { 4.f, 0.f };
    FConnectItBoardChangeEvent Change;
    Rules.StampWinState(Board, &Change);
    if (const auto* Checkpoint = OnlyEvent<FConnectItBoardEvent_WinCheckpointReached>(Change))
    {
        TestTrue(TEXT("the checkpoint carries the asset"), Checkpoint->Asset == ScoreGoal);
        TestEqual(TEXT("and its name"), Checkpoint->GroupName, FName(TEXT("WC_Test_Score4")));
    }
    else
    {
        AddError(TEXT("expected exactly one WinCheckpointReached event"));
    }

    // The search plays resolved goals exactly like the same groups built in code
    FConnectItRuleSet AssetRules;
    AssetRules.PlayerWinConditions = { ScoreGoal };
    AssetRules.ResolveWinGroups();
    const FConnectItRuleSet InlineRules = RulesWonAt(4.f);

    FConnectItBoardState SearchBoard = MakeBoard(7);
    Place(SearchBoard, 3, 0, 0);
    Place(SearchBoard, 3, 1, 0);
    Place(SearchBoard, 3, 2, 0);
    const FConnectItMinMaxRules FromAssets(SearchBoard, AssetRules, DefaultEvaluationWeights(), DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());
    const FConnectItMinMaxRules FromCode(SearchBoard, InlineRules, DefaultEvaluationWeights(), DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());
    const auto AssetResult = FConnectItMinMaxSearch::Run(FromAssets, FromAssets.MakeRoot(SearchBoard, 0), Depth(2));
    const auto CodeResult = FConnectItMinMaxSearch::Run(FromCode, FromCode.MakeRoot(SearchBoard, 0), Depth(2));
    if (!TestEqual(TEXT("same number of moves"), AssetResult.RootScores.Num(), CodeResult.RootScores.Num())) return false;
    bool bSame = true;
    for (int32 Index = 0; Index < AssetResult.RootScores.Num(); Index++)
    {
        bSame &= PlacePosition(AssetResult.RootScores[Index].Move) == PlacePosition(CodeResult.RootScores[Index].Move)
            && AssetResult.RootScores[Index].Score == CodeResult.RootScores[Index].Score;
    }
    TestTrue(TEXT("same ranking, same scores"), bSame);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAICoordinatorTest,
    "ConnectIt.AI.Coordinator", TestFlags)

bool FConnectItAICoordinatorTest::RunTest(const FString& Parameters)
{
    // No world here, so no timers: the test polls and ends the budget by hand
    using FFixed = UConnectIt_TestAIStrategy_FixedCandidates;
    using FSilent = UConnectIt_TestAIStrategy_Silent;

    const auto Fixed = [](int32 Row, int32 NumCandidates)
    {
        FFixed* Strategy = NewObject<FFixed>();
        Strategy->Row = Row;
        Strategy->NumCandidates = NumCandidates;
        Strategy->MaxCandidates = 10;
        return Strategy;
    };
    const auto RowOf = [](const FConnectItAICandidate& Candidate)
    {
        const FConnectItBoardOperation_PlacePiece* Place = Candidate.Payload.GetPtr<FConnectItBoardOperation_PlacePiece>();
        return Place ? Place->Position.X : INDEX_NONE;
    };
    UConnectIt_AISelector_FirstAvailable* First = NewObject<UConnectIt_AISelector_FirstAvailable>();
    First->MaxCandidates = 10;

    // Strategies answering at once: concluded straight away, priority order
    {
        UConnectIt_AICoordinator* Coordinator = NewObject<UConnectIt_AICoordinator>();
        FFixed* A = Fixed(0, 2);
        FFixed* B = Fixed(1, 2);
        Coordinator->Begin(FConnectItAIDecisionContext(), { A, B }, First, 2.f, 0.5f);

        TestFalse(TEXT("concluded once every strategy finished"), Coordinator->IsRunning());
        const FConnectItAIDecision& Decision = Coordinator->GetLastDecision();
        if (TestEqual(TEXT("all four candidates"), Decision.Candidates.Num(), 4))
        {
            TestEqual(TEXT("the first strategy's first"), RowOf(Decision.Candidates[0]), 0);
            TestEqual(TEXT("then the second's"), RowOf(Decision.Candidates[2]), 1);
            TestEqual(TEXT("labelled by strategy"), Decision.Candidates[2].SourceLabel,
                FName(TEXT("ConnectIt_TestAIStrategy_FixedCandidates")));
        }
        TestEqual(TEXT("each strategy gets the budget x share"), A->LastTimeBudget, 1.f);
    }

    // A slow strategy: waits while it works; the budget ending concludes with what there is
    {
        UConnectIt_AICoordinator* Coordinator = NewObject<UConnectIt_AICoordinator>();
        FSilent* Slow = NewObject<FSilent>();
        Coordinator->Begin(FConnectItAIDecisionContext(), { Slow, Fixed(1, 2) }, First, 2.f, 0.8f);
        Coordinator->Poll();
        TestTrue(TEXT("waits while a strategy is still working"), Coordinator->IsRunning());

        Coordinator->EndTimeBudget();
        TestFalse(TEXT("the budget running out concludes"), Coordinator->IsRunning());
        TestEqual(TEXT("with the answer that arrived"), Coordinator->GetLastDecision().Candidates.Num(), 2);
        TestEqual(TEXT("the slow strategy is stopped"), Slow->TimesCancelled, 1);
    }

    // A slow strategy's best so far is used, at its priority
    {
        UConnectIt_AICoordinator* Coordinator = NewObject<UConnectIt_AICoordinator>();
        FSilent* Slow = NewObject<FSilent>();
        Coordinator->Begin(FConnectItAIDecisionContext(), { Slow, Fixed(1, 2) }, First, 2.f, 0.8f);

        FConnectItBoardOperation_PlacePiece Place;
        Place.Position = FGridPosition(7, 7);
        Slow->Publish(UConnectIt_AIStrategy::MakeSingleMoveDecision(FInstancedStruct::Make(Place), TEXT("so far")));
        Coordinator->Poll();
        TestTrue(TEXT("a published answer doesn't end the wait"), Coordinator->IsRunning());

        Coordinator->EndTimeBudget();
        const FConnectItAIDecision& Decision = Coordinator->GetLastDecision();
        TestTrue(TEXT("the slow strategy's best so far comes first"),
            Decision.Candidates.Num() == 3 && RowOf(Decision.Candidates[0]) == 7);
    }

    // Nothing in by the budget: wait (warning) for the first answer
    {
        UConnectIt_AICoordinator* Coordinator = NewObject<UConnectIt_AICoordinator>();
        FSilent* SlowA = NewObject<FSilent>();
        FSilent* SlowB = NewObject<FSilent>();
        SlowA->Label = TEXT("SlowA");
        SlowB->Label = TEXT("SlowB");
        Coordinator->Begin(FConnectItAIDecisionContext(), { SlowA, SlowB }, First, 2.f, 0.8f);

        AddExpectedError(TEXT("Still working: SlowA, SlowB"), EAutomationExpectedErrorFlags::Contains, 1);
        Coordinator->EndTimeBudget();
        TestTrue(TEXT("nothing in yet: keeps waiting"), Coordinator->IsRunning());

        FConnectItBoardOperation_PlacePiece Place;
        Place.Position = FGridPosition(5, 5);
        SlowB->AnswerLate(UConnectIt_AIStrategy::MakeSingleMoveDecision(FInstancedStruct::Make(Place), TEXT("late")));
        Coordinator->Poll();
        TestFalse(TEXT("the first answer concludes"), Coordinator->IsRunning());
        TestTrue(TEXT("with that answer"),
            Coordinator->GetLastDecision().Candidates.Num() == 1 && RowOf(Coordinator->GetLastDecision().Candidates[0]) == 5);
        TestEqual(TEXT("the other strategy is stopped"), SlowA->TimesCancelled, 1);
    }

    // No budget: waits for everyone
    {
        UConnectIt_AICoordinator* Coordinator = NewObject<UConnectIt_AICoordinator>();
        FSilent* Slow = NewObject<FSilent>();
        Coordinator->Begin(FConnectItAIDecisionContext(), { Slow, Fixed(1, 2) }, First, 0.f, 0.8f);
        Coordinator->Poll();
        TestTrue(TEXT("no budget: still waiting"), Coordinator->IsRunning());
        Slow->AnswerLate(FConnectItAIDecision());
        Coordinator->Poll();
        TestFalse(TEXT("everyone finished"), Coordinator->IsRunning());
    }

    // Cancelling stops the strategies; no answer
    {
        UConnectIt_AICoordinator* Coordinator = NewObject<UConnectIt_AICoordinator>();
        FSilent* Slow = NewObject<FSilent>();
        Coordinator->Begin(FConnectItAIDecisionContext(), { Slow }, First, 2.f, 0.8f);
        Coordinator->Cancel();
        TestEqual(TEXT("a cancel stops the strategies"), Slow->TimesCancelled, 1);
        TestFalse(TEXT("and ends the decision"), Coordinator->IsRunning());
        TestFalse(TEXT("without an answer"), Coordinator->GetLastDecision().HasMove());
    }

    // The selector decides; none = first available
    {
        UConnectIt_AICoordinator* Coordinator = NewObject<UConnectIt_AICoordinator>();
        UConnectIt_AISelector_TopN* TopN = NewObject<UConnectIt_AISelector_TopN>();
        TopN->TopMovesConsidered = 2;
        TopN->MistakeChance = 1.f;
        Coordinator->Begin(FConnectItAIDecisionContext(), { Fixed(0, 3) }, TopN, 2.f, 0.8f);
        TestEqual(TEXT("the selector's pick comes first"),
            Coordinator->GetLastDecision().Candidates[0].Summary, FString(TEXT("1")));

        Coordinator->Begin(FConnectItAIDecisionContext(), { Fixed(0, 3) }, nullptr, 2.f, 0.8f);
        TestEqual(TEXT("no selector: first available"),
            Coordinator->GetLastDecision().Candidates[0].Summary, FString(TEXT("0")));
    }

    // No strategies: an empty answer
    {
        UConnectIt_AICoordinator* Coordinator = NewObject<UConnectIt_AICoordinator>();
        AddExpectedError(TEXT("has no strategies"), EAutomationExpectedErrorFlags::Contains, 1);
        Coordinator->Begin(FConnectItAIDecisionContext(), {}, First, 2.f, 0.8f);
        TestFalse(TEXT("no strategies, no move"), Coordinator->GetLastDecision().HasMove());
        TestFalse(TEXT("and done"), Coordinator->IsRunning());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAIDirectorTest,
    "ConnectIt.AI.Director", TestFlags)

bool FConnectItAIDirectorTest::RunTest(const FString& Parameters)
{
    // What can be checked without a running match: the default setup, events
    // reach a director subclass, changes wait for the next decision, and a
    // director with no AI controller changes no loadout. Turn and checkpoint
    // wiring is covered in PIE.
    UConnectIt_TestAIDirector* Director = NewObject<UConnectIt_TestAIDirector>();

    TestTrue(TEXT("the default director runs one MinMax"),
        Director->Strategies.Num() == 1 && Director->Strategies[0] && Director->Strategies[0]->IsA<UConnectIt_AIStrategy_MinMax>());
    TestTrue(TEXT("with First Available"), Director->Selector && Director->Selector->IsA<UConnectIt_AISelector_FirstAvailable>());

    Director->OnMatchStarted();
    Director->OnAITurnStarted(3);
    Director->OnAITurnEnded(3);
    Director->OnOpponentTurnStarted(4);
    Director->OnWinCheckpointReached(1, true, nullptr, 0, TEXT("Stage 1"));
    TestEqual(TEXT("events reach the director"), Director->Log,
        FString(TEXT("match; start 3; end 3; opponent 4; checkpoint 1 AI None 0 Stage 1; ")));

    // Strategy and selector changes wait for the next decision, as copies
    UConnectIt_AIStrategy* const OriginalStrategy = Director->Strategies[0];
    UConnectIt_TestAIStrategy_FixedCandidates* StrategyTemplate = NewObject<UConnectIt_TestAIStrategy_FixedCandidates>();
    UConnectIt_TestAIStrategy_Silent* SecondTemplate = NewObject<UConnectIt_TestAIStrategy_Silent>();
    UConnectIt_AISelector_GreedyScorer* SelectorTemplate = NewObject<UConnectIt_AISelector_GreedyScorer>();
    Director->TrySetStrategies({ StrategyTemplate, nullptr, SecondTemplate });
    Director->TrySetSelector(SelectorTemplate);
    TestTrue(TEXT("not before the next decision"), Director->Strategies[0] == OriginalStrategy);

    Director->ApplyPendingChanges();
    if (TestEqual(TEXT("the new strategies, nulls skipped"), Director->Strategies.Num(), 2))
    {
        TestTrue(TEXT("in order"), Director->Strategies[0]->IsA<UConnectIt_TestAIStrategy_FixedCandidates>()
            && Director->Strategies[1]->IsA<UConnectIt_TestAIStrategy_Silent>());
        TestTrue(TEXT("copies, not the templates"), Director->Strategies[0] != StrategyTemplate);
        TestTrue(TEXT("owned by the director"), Director->Strategies[0]->GetOuter() == Director);
    }
    TestTrue(TEXT("the new selector, copied"),
        Director->Selector && Director->Selector->IsA<UConnectIt_AISelector_GreedyScorer>() && Director->Selector != SelectorTemplate);

    Director->ApplyPendingChanges();
    TestEqual(TEXT("applying again changes nothing"), Director->Strategies.Num(), 2);

    Director->TrySetStrategies({});
    Director->ApplyPendingChanges();
    TestEqual(TEXT("an empty list empties it"), Director->Strategies.Num(), 0);

#if WITH_EDITOR
    // Validation (run by the preset holding it): no loadout and no strategies can't play
    {
        FDataValidationContext Validation;
        TestTrue(TEXT("no starting loadout, no strategies: invalid"),
            Director->IsDataValid(Validation) == EDataValidationResult::Invalid);
        TestEqual(TEXT("both reported"), static_cast<int32>(Validation.GetNumErrors()), 2);
    }
#endif

    AddExpectedError(TEXT("has no AI controller"), EAutomationExpectedErrorFlags::Contains, 1);
    Director->TrySetLoadout(nullptr);
    TestNull(TEXT("no controller, no loadout"), Director->TryGetLoadout());

    // A match preset can carry one
    UConnectIt_MatchPreset* Preset = NewObject<UConnectIt_MatchPreset>();
    Preset->AIDirector = NewObject<UConnectIt_TestAIDirector>(Preset);
    TestNotNull(TEXT("a preset holds its director"), Preset->AIDirector.Get());
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
