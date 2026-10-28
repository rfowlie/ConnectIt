// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "MinMax/ConnectIt_MinMaxRules.h"
#include "AI/ConnectIt_AIStrategy_MinMax.h"
#include "AI/ConnectIt_AIDirector.h"
#include "Framework/Data/ConnectIt_MatchPresetDataAsset.h"
#include "Framework/Data/ConnectIt_BoardLayoutDataAsset.h"
#include "AI/ConnectIt_AICoordinator.h"
#include "Framework/GameMode/ConnectIt_LevelChange.h"
#include "AI/ConnectIt_MinMaxSearch.h"
#include "AI/ConnectIt_AISelector.h"
#include "ConnectIt_MinMaxTestTerms.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Board/Rules/Conditions/ConnectIt_MatchConditionDataAsset.h"
#include "Framework/Library/ConnectIt_BoardStateLibrary.h"
#include "Board/Rules/Scoring/ConnectIt_LineScoringRule.h"
#include "Board/Rules/Conditions/ConnectIt_MatchConditions.h"
#include "Board/Operations/ConnectIt_BoardOperations.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "ConnectIt_GameplayTags.h"
#include "Misc/DataValidation.h"
#include "Misc/ScopeExit.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Board/Actor/ConnectIt_BuilderTile.h"
#include "Board/Powers/ConnectIt_TileCatalogDataAsset.h"
#include "Board/Powers/ConnectIt_TilePowerEffects.h"
#include "Board/ConnectIt_TileReaction.h"
#include "Board/ConnectIt_TileVisualizer.h"
#include "AI/ConnectIt_LevelDirector.h"
#include "NativeGameplayTags.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ConnectItMinMaxTests
{
    using namespace GameIntelligence::Search::MinMax;

    constexpr EAutomationTestFlags TestFlags =
        EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

    // Test-only tile power names (ConnectIt.Board.TilePowerData)
    UE_DEFINE_GAMEPLAY_TAG_STATIC(TestTilePowerTag, "ConnectIt.TilePower.Test");
    UE_DEFINE_GAMEPLAY_TAG_STATIC(TestOtherTilePowerTag, "ConnectIt.TilePower.TestOther");
    UE_DEFINE_GAMEPLAY_TAG_STATIC(TestUndefinedTilePowerTag, "ConnectIt.TilePower.TestUndefined");
    UE_DEFINE_GAMEPLAY_TAG_STATIC(TestPower1, "ConnectIt.TilePower.Test.1");
    UE_DEFINE_GAMEPLAY_TAG_STATIC(TestPower2, "ConnectIt.TilePower.Test.2");
    UE_DEFINE_GAMEPLAY_TAG_STATIC(TestPower3, "ConnectIt.TilePower.Test.3");
    UE_DEFINE_GAMEPLAY_TAG_STATIC(TestPower4, "ConnectIt.TilePower.Test.4");
    UE_DEFINE_GAMEPLAY_TAG_STATIC(TestPower5, "ConnectIt.TilePower.Test.5");

    // A test power: Name, Trigger and Effect. A test effect is given Name and
    // the gate; a shipped effect names its own tag (Name must be it)
    template<typename TEffect>
    FConnectItTilePower MakeTestPower(FGameplayTag Name, EConnectItTilePowerTrigger Trigger, const TEffect& Effect,
        int32 CooldownTurns = 0, float FireAtMultiplier = 0.f)
    {
        TEffect Named = Effect;
        if constexpr (std::is_base_of_v<FConnectItTilePowerEffect_TestBase, TEffect>)
        {
            Named.TestTag = Name;
            Named.Gate.FireAtMultiplier = FireAtMultiplier;
        }
        else
        {
            check(Named.GetPowerTag() == Name && FireAtMultiplier == 0.f);
        }

        FConnectItTilePower Power;
        Power.SetTrigger(Trigger);
        Power.CooldownTurns = CooldownTurns;
        Power.Effect = TInstancedStruct<FConnectItTilePowerEffect>::Make<TEffect>(Named);
        return Power;
    }

    FConnectItTilePowerEffect_TestPlace PlaceAt(std::initializer_list<FIntPoint> Offsets)
    {
        FConnectItTilePowerEffect_TestPlace Effect;
        Effect.Offsets = Offsets;
        return Effect;
    }

    void SetPower(FConnectItBoardState& Board, int32 X, int32 Y, FGameplayTag Name)
    {
        Board.GetTileDataMutable(FGridPosition(X, Y))->TilePowerTag = Name;
    }

    // Every Power Triggered event of a change, in order
    TArray<const FConnectItBoardEvent_TilePowerTriggered*> PowersTriggered(const FConnectItBoardChangeEvent& Change)
    {
        TArray<const FConnectItBoardEvent_TilePowerTriggered*> Found;
        for (const FInstancedStruct& Entry : Change.Events)
        {
            if (const auto* Triggered = Entry.GetPtr<FConnectItBoardEvent_TilePowerTriggered>()) Found.Add(Triggered);
        }
        return Found;
    }

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

    // Whether a change-record list has an entry at Position
    template<typename TItem, typename TAllocator>
    bool HasPosition(const TArray<TItem, TAllocator>& Items, const FGridPosition& Position)
    {
        return Items.ContainsByPredicate([&Position](const TItem& Item) { return Item.Position == Position; });
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
        return FGameplayTagContainer(ConnectIt_BoardOperation_PlacePiece);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItNoLegalMoveTest,
    "ConnectIt.Board.Conditions.NoLegalMove", TestFlags)

bool FConnectItNoLegalMoveTest::RunTest(const FString& Parameters)
{
    const FConnectItRuleSet Rules;
    FConnectItMatchCondition_NoLegalMove NoPlace;
    NoPlace.Move = ConnectIt_BoardOperation_PlacePiece;
    FConnectItMatchCondition_NoLegalMove NoSwap;
    NoSwap.Move = ConnectIt_BoardOperation_SwapPieces;

    // Place: a full board has none, one free tile is enough
    FConnectItBoardState Full = MakeBoard(2);
    Full.ScoreBoard = { 0.f, 0.f };
    Place(Full, 0, 0, 0);
    Place(Full, 0, 1, 0);
    Place(Full, 1, 0, 1);
    const FConnectItConditionContext OneFree{ Full, Rules };
    TestFalse(TEXT("one free tile: a legal place"), NoPlace.IsMetFor(OneFree, 0));
    TestTrue(TEXT("the helper agrees"),
        UConnectIt_BoardStateLibrary::HasAnyLegalMove(Full, ConnectIt_BoardOperation_PlacePiece, 0) == EConnectItMoveLegality::Legal);
    Place(Full, 1, 1, 1);
    const FConnectItConditionContext NoneFree{ Full, Rules };
    TestTrue(TEXT("a full board: no legal place"), NoPlace.IsMetFor(NoneFree, 0));
    TestTrue(TEXT("the helper agrees"),
        UConnectIt_BoardStateLibrary::HasAnyLegalMove(Full, ConnectIt_BoardOperation_PlacePiece, 0) == EConnectItMoveLegality::NoLegalMove);

    // Swap: needs a piece of its own and another piece to trade with
    FConnectItBoardState Board = MakeBoard(5);
    Board.ScoreBoard = { 0.f, 0.f };
    Place(Board, 0, 0, 0);
    Place(Board, 1, 0, 0);
    const FConnectItConditionContext Ctx{ Board, Rules };
    TestTrue(TEXT("no opposing piece: no legal swap"), NoSwap.IsMetFor(Ctx, 0));
    TestTrue(TEXT("no own piece: no legal swap"), NoSwap.IsMetFor(Ctx, 1));
    Place(Board, 4, 4, 1);
    TestFalse(TEXT("one of each: a legal swap"), NoSwap.IsMetFor(Ctx, 0));
    TestFalse(TEXT("...for both"), NoSwap.IsMetFor(Ctx, 1));
    TestFalse(TEXT("an unknown faction never"), NoSwap.IsMetFor(Ctx, 5));

    // A kind of move that can't tell yet: Unknown, never met, and validation says so
    FConnectItMatchCondition_NoLegalMove NoShift;
    NoShift.Move = ConnectIt_BoardOperation_Shift;
    TestTrue(TEXT("shift can't tell yet"),
        FConnectItBoardOperation::GetLegality(ConnectIt_BoardOperation_Shift, Board, 0) == EConnectItMoveLegality::Unknown);
    TestFalse(TEXT("so No Legal Move: Shift never holds"), NoShift.IsMetFor(Ctx, 0));
    TArray<FText> Warnings;
    NoShift.GetValidationWarnings(nullptr, Warnings);
    TestEqual(TEXT("...and validation warns"), Warnings.Num(), 1);
    Warnings.Reset();
    NoSwap.GetValidationWarnings(nullptr, Warnings);
    TestEqual(TEXT("no warning for swap"), Warnings.Num(), 0);
    FConnectItMatchCondition_NoLegalMove Unset;
    Unset.GetValidationWarnings(nullptr, Warnings);
    TestEqual(TEXT("no Move set: warned"), Warnings.Num(), 1);
    TestTrue(TEXT("a tag that isn't a move: Unknown"),
        FConnectItBoardOperation::GetLegality(FGameplayTag(), Board, 0) == EConnectItMoveLegality::Unknown);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItLoseConditionsTest,
    "ConnectIt.Board.Conditions.Lose", TestFlags)

bool FConnectItLoseConditionsTest::RunTest(const FString& Parameters)
{
    // The player wins by clearing the board having scored; loses when stuck
    // with fewer pieces than a line needs. The AI wins at 12 points.
    UConnectIt_MatchConditionDataAsset* Clear = NewObject<UConnectIt_MatchConditionDataAsset>(GetTransientPackage(), TEXT("WC_Test_LoseClear"));
    Clear->Conditions.Add(TInstancedStruct<FConnectItMatchCondition>::Make<FConnectItMatchCondition_PieceCount>(PiecesIs(EConnectItPieceCountComparison::Equal, 0)));
    UConnectIt_MatchConditionDataAsset* Stuck = NewObject<UConnectIt_MatchConditionDataAsset>(GetTransientPackage(), TEXT("WC_Test_LoseStuck"));
    Stuck->Conditions.Add(TInstancedStruct<FConnectItMatchCondition>::Make<FConnectItMatchCondition_NotEnoughPiecesToScore>());
    UConnectIt_MatchConditionDataAsset* AIScore = NewObject<UConnectIt_MatchConditionDataAsset>(GetTransientPackage(), TEXT("WC_Test_LoseAIScore"));
    AIScore->Conditions.Add(TInstancedStruct<FConnectItMatchCondition>::Make<FConnectItMatchCondition_ScoreThreshold>(ScoreAtLeast(12.f)));

    FConnectItRuleSet Rules;
    Rules.ResolveConditionGroups({ Clear }, { AIScore }, { Stuck }, {});
    Rules.AIFaction = 1;

    // Player (0) has 2 pieces left: stuck, so it loses and the AI wins
    FConnectItBoardState Board = MakeBoard(5);
    Board.ScoreBoard = { 0.f, 0.f };
    Board.StartingPieces = { 4, 1 };
    Place(Board, 0, 0, 0);
    Place(Board, 1, 0, 0);
    Place(Board, 4, 4, 1);

    TestEqual(TEXT("the player is the loser"), Rules.GetLosingFaction(Board), 0);
    TestEqual(TEXT("so the AI is the winner"), Rules.GetWinningFaction(Board), 1);

    FConnectItBoardChangeEvent Change;
    Rules.StampWinState(Board, &Change);
    TestTrue(TEXT("game over"), Board.bGameOver);
    TestEqual(TEXT("won by the AI"), Board.WinningFactionSlot, 1);
    TestEqual(TEXT("lost by the player"), Board.LosingFactionSlot, 0);
    if (const auto* Lost = OnlyEvent<FConnectItBoardEvent_LoseConditionMet>(Change))
    {
        TestEqual(TEXT("the event names the loser"), Lost->Faction, 0);
        TestTrue(TEXT("and the condition met"), Lost->Asset == Stuck);
    }
    else
    {
        AddError(TEXT("expected exactly one LoseConditionMet event"));
    }

    // 0 pieces: the "clear the board" goal is complete -- a win, even though
    // having no pieces also means "not enough to score"
    FConnectItBoardState Cleared = MakeBoard(5);
    Cleared.ScoreBoard = { 4.f, 0.f };
    Cleared.StartingPieces = { 4, 1 };
    Place(Cleared, 4, 4, 1);
    FConnectItBoardChangeEvent ClearedChange;
    Rules.StampWinState(Cleared, &ClearedChange);
    TestEqual(TEXT("win beats loss: the player wins"), Cleared.WinningFactionSlot, 0);
    TestEqual(TEXT("...no loser"), Cleared.LosingFactionSlot, -1);
    TestTrue(TEXT("...no Lose event"), !ClearedChange.Events.ContainsByPredicate(
        [](const FInstancedStruct& Entry) { return Entry.GetPtr<FConnectItBoardEvent_LoseConditionMet>() != nullptr; }));

    // No AI in the match: lose conditions are ignored
    FConnectItRuleSet Online = Rules;
    Online.AIFaction = INDEX_NONE;
    TestEqual(TEXT("online: nobody loses"), Online.GetLosingFaction(Board), static_cast<int32>(INDEX_NONE));

    // The AI's search sees a loss as the end of the game
    FConnectItBoardState Fresh = MakeBoard(5);
    Fresh.ScoreBoard = { 0.f, 0.f };
    Fresh.StartingPieces = { 4, 1 };
    Place(Fresh, 0, 0, 0);
    Place(Fresh, 1, 0, 0);
    Place(Fresh, 4, 4, 1);
    const FConnectItMinMaxRules Search(Fresh, Rules, DefaultEvaluationWeights(), DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());
    TestTrue(TEXT("a lost position is terminal"), Search.IsTerminalState(Search.MakeRoot(Fresh, 1)));
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
    const float Points = RuleSet.ApplyScoring(GameBoard, FGridPosition(3, 3), true);

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

    FConnectItBoardChanges Touched;
    FConnectItBoardChangeEvent Event;
    Free.Apply(Board, Touched, &Event);
    TestEqual(TEXT("the piece is placed"), TileAt(Board, 0, 0).FactionPiece, 0);
    TestTrue(TEXT("exactly the placed position is reported, as a piece entering"),
        Touched.Entered.Num() == 1 && Touched.Entered[0].Position == FGridPosition(0, 0)
        && Touched.Entered[0].Faction == 0 && Touched.Entered[0].bSurvivesScoring);
    TestTrue(TEXT("...and nothing else"), Touched.Exited.IsEmpty() && Touched.Moved.IsEmpty()
        && Touched.PieceChanged.IsEmpty() && Touched.TileChanged.IsEmpty());
    TestEqual(TEXT("applying an operation does not score"), Board.GetScore(0), 0.f);
    const auto* Placed = OnlyEvent<FConnectItBoardEvent_PiecePlaced>(Event);
    TestTrue(TEXT("one Piece Placed event says where and by whom"),
        Placed && Placed->Position == FGridPosition(0, 0) && Placed->Faction == 0);

    TestEqual(TEXT("its operation tag is Place Piece"),
        Free.GetOperationTag(), FGameplayTag(ConnectIt_BoardOperation_PlacePiece));
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

    FConnectItBoardChanges Touched;
    FConnectItBoardChangeEvent Event;
    Trade.Apply(Board, Touched, &Event);
    TestEqual(TEXT("faction 1's piece is now on (0,0)"), TileAt(Board, 0, 0).FactionPiece, 1);
    TestEqual(TEXT("faction 0's piece is now on (3,3)"), TileAt(Board, 3, 3).FactionPiece, 0);
    TestTrue(TEXT("each tile's piece left and the other arrived"),
        Touched.Exited.Num() == 2 && Touched.Entered.Num() == 2
        && Touched.Entered[0].Position == Stray && Touched.Entered[0].Faction == 1
        && Touched.Exited[0].Position == Stray && Touched.Exited[0].Faction == 0);
    TestTrue(TEXT("...arriving pieces that don't survive scoring"),
        !Touched.Entered[0].bSurvivesScoring && !Touched.Entered[1].bSurvivesScoring);
    TestEqual(TEXT("applying an operation does not score"), Board.GetScore(0), 0.f);
    const auto* Swapped = OnlyEvent<FConnectItBoardEvent_PiecesSwapped>(Event);
    TestTrue(TEXT("one Pieces Swapped event says which two"),
        Swapped && Swapped->PositionA == Stray && Swapped->PositionB == Enemy);

    // The separate after-move step is what scores the completed line
    const FConnectItRuleSet Rules;
    TestFalse(TEXT("a swap's arriving pieces don't survive scoring"), Trade.ArrivingPiecesSurviveScoring());
    TestTrue(TEXT("a placement's do"), FConnectItBoardOperation_PlacePiece().ArrivingPiecesSurviveScoring());

    const float Points = Rules.ResolveBoardChange(Board, Touched, &Event);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItScoringPassTest,
    "ConnectIt.Board.ScoringPass", TestFlags)

bool FConnectItScoringPassTest::RunTest(const FString& Parameters)
{
    // One pass finds every line from every arrival on the same board, then
    // applies them all -- so two arrivals can't break each other's lines
    const FConnectItRuleSet Rules;   // Lines of 4

    {
        // Two arrivals, (0,3) and (3,6), each completing a line; the lines
        // share (3,3), which neither arrival is on. One at a time, the first
        // line would clear (3,3) and break the second.
        FConnectItBoardState Board = MakeBoard(7);
        Place(Board, 0, 3, 0); Place(Board, 1, 3, 0); Place(Board, 2, 3, 0); Place(Board, 3, 3, 0);
        Place(Board, 3, 4, 0); Place(Board, 3, 5, 0); Place(Board, 3, 6, 0);

        FConnectItBoardChanges Shifted;   // as a shift reports them: moved
        Shifted.AddMoved(FGridPosition(0, 3), 0, true);
        Shifted.AddMoved(FGridPosition(3, 6), 0, true);
        FConnectItBoardChangeEvent Events;
        const float Points = Rules.ResolveBoardChange(Board, Shifted, &Events);

        TestEqual(TEXT("shared tile: both lines score"), Events.Events.Num(), 2);
        // The first line raises (3,3) to 2 before the second sums it: 4 + 5
        TestEqual(TEXT("shared tile: 4 + 5 points"), Points, 9.f);
        TestEqual(TEXT("shared tile: the score"), Board.GetScore(0), 9.f);
        TestTrue(TEXT("shared tile: each arrival keeps its piece"),
            TileAt(Board, 0, 3).FactionPiece == 0 && TileAt(Board, 3, 6).FactionPiece == 0);
        TestEqual(TEXT("shared tile: the shared piece is cleared"), TileAt(Board, 3, 3).FactionPiece, -1);
        TestEqual(TEXT("shared tile: its multiplier rose once per line"), TileAt(Board, 3, 3).Multiplier, 3.f);
    }

    {
        // Two arrivals on the same line: it counts once, and keeps only the
        // first arrival that survives
        FConnectItBoardState Board = MakeBoard(7);
        Place(Board, 0, 0, 0); Place(Board, 1, 0, 0); Place(Board, 2, 0, 0); Place(Board, 3, 0, 0);

        FConnectItBoardChanges Shifted;
        Shifted.AddMoved(FGridPosition(1, 0), 0, true);
        Shifted.AddMoved(FGridPosition(2, 0), 0, true);
        FConnectItBoardChangeEvent Events;
        const float Points = Rules.ResolveBoardChange(Board, Shifted, &Events);

        TestEqual(TEXT("same line: scored once"), Events.Events.Num(), 1);
        TestEqual(TEXT("same line: 4 points"), Points, 4.f);
        TestTrue(TEXT("same line: only the first arrival stays"),
            TileAt(Board, 1, 0).FactionPiece == 0 && TileAt(Board, 2, 0).FactionPiece == -1);
    }

    {
        // Each arrival keeps its own survival: a swapped-in piece (doesn't
        // survive) and a placed one (does), in one pass
        FConnectItBoardState Board = MakeBoard(7);
        Place(Board, 0, 0, 0); Place(Board, 1, 0, 0); Place(Board, 2, 0, 0); Place(Board, 3, 0, 0);
        Place(Board, 0, 5, 1); Place(Board, 1, 5, 1); Place(Board, 2, 5, 1); Place(Board, 3, 5, 1);

        FConnectItBoardChanges Mixed;
        Mixed.AddEntered(FGridPosition(3, 0), 0, false);
        Mixed.AddEntered(FGridPosition(3, 5), 1, true);
        Rules.ResolveBoardChange(Board, Mixed);

        TestEqual(TEXT("mixed: the swapped-in piece is cleared"), TileAt(Board, 3, 0).FactionPiece, -1);
        TestEqual(TEXT("mixed: the placed piece stays"), TileAt(Board, 3, 5).FactionPiece, 1);
        TestTrue(TEXT("mixed: both factions score"), Board.GetScore(0) == 4.f && Board.GetScore(1) == 4.f);
    }

    {
        // What a scoring pass reports: where its lines ran, and the
        // multipliers it raised
        FConnectItBoardState Board = MakeBoard(7);
        Place(Board, 0, 0, 0); Place(Board, 1, 0, 0); Place(Board, 2, 0, 0); Place(Board, 3, 0, 0);
        const FConnectItScoringRule* Rule = Rules.GetScoringRule();
        const FConnectItPieceArrival Arrival{ FGridPosition(3, 0), 0, true };
        FConnectItBoardChanges Scored;
        Rule->ScorePass(Board, MakeArrayView(&Arrival, 1), Scored, nullptr);
        TestEqual(TEXT("record: the line's four tiles"), Scored.ScoredThrough.Num(), 4);
        TestTrue(TEXT("record: each multiplier change, for the scoring faction"),
            Scored.TileChanged.Num() == 4 && Scored.TileChanged[0].What == EConnectItTileChange::Multiplier
            && Scored.TileChanged[0].Faction == 0);
        TestFalse(TEXT("record: cleared pieces are not exits"), Scored.HasScoringArrivals() || !Scored.Exited.IsEmpty());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItChangeRecordTest,
    "ConnectIt.Board.ChangeRecord", TestFlags)

bool FConnectItChangeRecordTest::RunTest(const FString& Parameters)
{
    // What each remaining operation reports (Place, Force Place, Swap, Shift,
    // Capture, Remove and Remove Tiles are checked in their own tests)
    FConnectItBoardState Board = MakeBoard(3);
    Board.GetTileDataMutable(FGridPosition(1, 1))->Multiplier = 3.f;

    {
        FConnectItBoardChanges Changes;
        MakeAt<FConnectItBoardOperation_DestroyTileMultiplier>(1, 1, 1).Apply(Board, Changes, nullptr);
        TestTrue(TEXT("destroy multiplier: the tile changed, by the operation's faction"),
            Changes.TileChanged.Num() == 1 && Changes.TileChanged[0].What == EConnectItTileChange::Multiplier
            && Changes.TileChanged[0].Faction == 1 && !Changes.HasScoringArrivals());
    }
    {
        FConnectItBoardChanges Changes;
        MakeAt<FConnectItBoardOperation_ToggleTileActive>(INDEX_NONE, 2, 2).Apply(Board, Changes, nullptr);
        TestTrue(TEXT("toggle: the tile changed (active)"),
            Changes.TileChanged.Num() == 1 && Changes.TileChanged[0].What == EConnectItTileChange::Active);
    }
    {
        FConnectItBoardOperation_AddTiles Add;
        FConnectItBoardLayoutCell WithPiece;
        WithPiece.Position = FGridPosition(5, 5);
        WithPiece.TileData.StartingFactionPiece = 0;
        FConnectItBoardLayoutCell Empty;
        Empty.Position = FGridPosition(5, 6);
        Add.Cells = { WithPiece, Empty };
        FConnectItBoardChanges Changes;
        Add.Apply(Board, Changes, nullptr);
        TestTrue(TEXT("add tiles: the starting piece is scored (moved) and sets off no enter"),
            Changes.Moved.Num() == 1 && Changes.Moved[0].Position == FGridPosition(5, 5) && Changes.Entered.IsEmpty());
    }
    {
        FConnectItBoardOperation_ChangeGoals Goals;
        Goals.Factions = { 0 };
        FConnectItBoardChanges Changes;
        Goals.Apply(Board, Changes, nullptr);
        FConnectItBoardOperation_BeginPhase Phase;
        Phase.Phase = FGameplayTag::RequestGameplayTag(TEXT("ConnectIt.Phase.1"), false);
        Phase.Apply(Board, Changes, nullptr);
        TestTrue(TEXT("change goals, begin phase: nothing"), Changes.IsEmpty());
    }
    {
        // Carrying out stamps the operation's faction on the record
        const FConnectItRuleSet Rules;
        FConnectItBoardState Fresh = MakeBoard(3);
        FConnectItBoardChangeEvent Events;
        Rules.CarryOutOperation(MakeAt<FConnectItBoardOperation_PlacePiece>(1, 0, 0), Fresh, &Events);
        TestEqual(TEXT("carry out: the placement happened"), TileAt(Fresh, 0, 0).FactionPiece, 1);
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
    FConnectItBoardChanges Touched;
    Completing.Apply(Board, Touched, nullptr);

    FConnectItBoardChangeEvent Change;
    const float Points = Rules.ResolveBoardChange(Board, Touched, &Change);

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
    FConnectItBoardChanges QuietTouched;
    Completing.Apply(Quiet, QuietTouched, nullptr);
    TestEqual(TEXT("same points without asking for events"),
        Rules.ResolveBoardChange(Quiet, QuietTouched), Points);

    // The same cross, completed by a piece that does NOT survive scoring
    // (what a swap's arriving piece does)
    FConnectItBoardState Cleared = MakeCross();
    FConnectItBoardChanges ClearedTouched;
    Completing.Apply(Cleared, ClearedTouched, nullptr);
    ClearedTouched.Entered[0].bSurvivesScoring = false;
    FConnectItBoardChangeEvent ClearedChange;
    const float ClearedPoints = Rules.ResolveBoardChange(Cleared, ClearedTouched, &ClearedChange);

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
    // Side 0 was granted no operation tags; side 1 can place
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

    FConnectItBoardChanges Touched;
    FConnectItBoardChangeEvent Event;
    MakeAt<FForce>(0, 1, 1).Apply(Board, Touched, &Event);
    TestEqual(TEXT("the piece there is replaced"), TileAt(Board, 1, 1).FactionPiece, 0);
    TestTrue(TEXT("the position is reported for scoring"),
        Touched.Entered.Num() == 1 && Touched.Entered[0].Position == FGridPosition(1, 1));
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

    FConnectItBoardChanges Touched;
    FConnectItBoardChangeEvent Event;
    MakeAt<FCapture>(0, 1, 1).Apply(Board, Touched, &Event);
    TestEqual(TEXT("the piece changes owner"), TileAt(Board, 1, 1).FactionPiece, 0);
    TestTrue(TEXT("the position is reported for scoring"),
        Touched.PieceChanged.Num() == 1 && Touched.PieceChanged[0].Position == FGridPosition(1, 1)
        && Touched.Entered.IsEmpty() && Touched.Exited.IsEmpty());
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

    FConnectItBoardChanges Touched;
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
        Touched.Moved.Num() == 2 && HasPosition(Touched.Moved, FGridPosition(1, 0)) && HasPosition(Touched.Moved, FGridPosition(3, 0))
        && Touched.Entered.IsEmpty() && Touched.Exited.IsEmpty());
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

    FConnectItBoardChanges Touched;
    FConnectItBoardChangeEvent Event;
    Shift.Apply(Board, Touched, &Event);

    TestEqual(TEXT("X=2 holds what X=0 held (faction 0)"), TileAt(Board, 2, 0).FactionPiece, 0);
    TestEqual(TEXT("X=3 holds what X=1 held (faction 1)"), TileAt(Board, 3, 0).FactionPiece, 1);
    TestEqual(TEXT("X=1 holds what X=4 held (multiplier 3, wrapped past the end)"),
        TileAt(Board, 1, 0).Multiplier, 3.f);
    TestEqual(TEXT("X=0 holds what X=3 held (empty, multiplier 1)"), TileAt(Board, 0, 0).Multiplier, 1.f);

    TestTrue(TEXT("both pieces are reported for scoring"),
        Touched.Moved.Num() == 2 && HasPosition(Touched.Moved, FGridPosition(2, 0)) && HasPosition(Touched.Moved, FGridPosition(3, 0)));

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

    FConnectItBoardChanges PinnedTouched;
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

    FConnectItBoardChanges Touched;
    FConnectItBoardChangeEvent Event;
    Remove.Apply(Board, Touched, &Event);
    TestTrue(TEXT("the piece is gone"),
        TileAt(Board, 1, 1).FactionPiece == -1 && !TileAt(Board, 1, 1).bIsOccupied);
    TestFalse(TEXT("nothing is reported for scoring"), Touched.HasScoringArrivals());
    TestTrue(TEXT("the piece leaving is reported, with its owner"),
        Touched.Exited.Num() == 1 && Touched.Exited[0].Position == FGridPosition(1, 1) && Touched.Exited[0].Faction == 1);
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

    FConnectItBoardChanges Touched;
    FConnectItBoardChangeEvent Event;
    MakeAt<FDestroy>(0, 1, 1).Apply(Board, Touched, &Event);
    TestEqual(TEXT("the multiplier is back to 1"), TileAt(Board, 1, 1).Multiplier, 1.f);
    TestFalse(TEXT("nothing is reported for scoring"), Touched.HasScoringArrivals());
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

    FConnectItBoardChanges Touched;
    FConnectItBoardChangeEvent Event;
    Toggle.Apply(Board, Touched, &Event);
    TestFalse(TEXT("an active tile becomes inactive"), TileAt(Board, 1, 1).bIsActive);
    TestFalse(TEXT("nothing is reported for scoring"), Touched.HasScoringArrivals());
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

    FConnectItBoardChanges Touched;
    MakeAt<FConnectItBoardOperation_PlacePiece>(0, 3, 3).Apply(Board, Touched, &Change);
    Rules.ResolveBoardChange(Board, Touched, &Change);

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
        return Rules.ApplyScoring(Board, FGridPosition(X + 3 * DX, Y + 3 * DY), true);
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
        return FLibrary::SimulateOperation(Board, Rules, FInstancedStruct::Make(Operation), Faction, -1,
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
        FLibrary::SimulateOperation(Board, Rules, FInstancedStruct::Make(FGridPosition(0, 0)), 0, -1,
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
    Context.AvailableOperationTags.AddTag(ConnectIt_BoardOperation_PlacePiece);
    TestTrue(TEXT("a place move is available"),
        UConnectIt_AIStrategy::IsOperationAvailable(Context, FInstancedStruct::Make(Place)));
    TestFalse(TEXT("a swap is not"),
        UConnectIt_AIStrategy::IsOperationAvailable(Context, FInstancedStruct::Make(FConnectItBoardOperation_SwapPieces())));

    // Every kind of board operation is found
    const TArray<FGameplayTag>& AllTypes = FConnectItBoardOperation::GetAllOperationTags();
    TestTrue(TEXT("all operation tags include place, swap and shift"),
        AllTypes.Contains(FGameplayTag(ConnectIt_BoardOperation_PlacePiece))
        && AllTypes.Contains(FGameplayTag(ConnectIt_BoardOperation_SwapPieces))
        && AllTypes.Contains(FGameplayTag(ConnectIt_BoardOperation_Shift)));
    // The eight player-facing ones and the level's Change Goals, Begin
    // Phase, Remove Tiles, Add Tiles, Change Layout, Set Tile Locked, Set
    // Chain Partner, Move Piece, Adjust Multiplier and Turn Start
    TestEqual(TEXT("one per kind of operation"), AllTypes.Num(), 18);
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
    Context.AvailableOperationTags.AddTag(ConnectIt_BoardOperation_PlacePiece);

    FConnectItMinMaxSearchSettings Settings;
    Settings.MaxDepth = 1;
    Settings.DefaultTimeBudgetSeconds = 0.f;
    Settings.OpponentOperationTags = PlaceOnly();

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
    SwapOnly.OwnOperationTags.AddTag(ConnectIt_BoardOperation_SwapPieces);
    TestFalse(TEXT("a side with only swaps can't be searched"),
        ConnectItMinMaxSearch::Prepare(Context, SwapOnly, Prepared, &WhyNot));

    FConnectItAIDecisionContext NothingAvailable = Context;
    NothingAvailable.AvailableOperationTags.Reset();
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
        GetDefault<UConnectIt_AIStrategy_MinMax>()->GetSupportedOperationTags().HasTagExact(ConnectIt_BoardOperation_PlacePiece));
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
    Context.AvailableOperationTags.AddTag(ConnectIt_BoardOperation_PlacePiece);

    FConnectItMinMaxSearchSettings Settings;
    Settings.MaxDepth = 3;
    Settings.DefaultTimeBudgetSeconds = 0.f;
    Settings.OpponentOperationTags = PlaceOnly();

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
    Context.AvailableOperationTags.AddTag(ConnectIt_BoardOperation_PlacePiece);

    UConnectIt_AIStrategy_MinMax* Strategy = NewObject<UConnectIt_AIStrategy_MinMax>();
    Strategy->Search.MaxDepth = 10;
    Strategy->Search.DefaultTimeBudgetSeconds = 0.f;
    Strategy->Search.OpponentOperationTags = PlaceOnly();
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
    UConnectIt_MatchConditionDataAsset* ScoreGoal = NewObject<UConnectIt_MatchConditionDataAsset>(
        GetTransientPackage(), TEXT("WC_Test_Score4"));
    ScoreGoal->Conditions.Add(TInstancedStruct<FConnectItMatchCondition>::Make<FConnectItMatchCondition_ScoreThreshold>(ScoreAtLeast(4.f)));
    UConnectIt_MatchConditionDataAsset* ClearGoal = NewObject<UConnectIt_MatchConditionDataAsset>(
        GetTransientPackage(), TEXT("WC_Test_Clear"));
    ClearGoal->Conditions.Add(TInstancedStruct<FConnectItMatchCondition>::Make<FConnectItMatchCondition_PieceCount>(PiecesIs(EConnectItPieceCountComparison::Equal, 0)));

    FConnectItRuleSet Rules;
    TestEqual(TEXT("nothing to play with before resolving"), Rules.PlayerWinGroups.Num(), 0);

    // As the GameMode does with the Level director's Player / AI Win Conditions
    AddExpectedError(TEXT("Player win condition 1 is empty"), EAutomationExpectedErrorFlags::Contains, 1);
    Rules.ResolveConditionGroups({ ScoreGoal, nullptr, ClearGoal }, { ScoreGoal });
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
    AssetRules.ResolveConditionGroups({ ScoreGoal }, {});
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
    Director->OnTurnStarted(1, true, 3);
    Director->OnTurnEnded(1, true, 3);
    Director->OnTurnStarted(0, false, 4);
    Director->OnWinCheckpointReached(1, true, nullptr, 0, TEXT("Stage 1"));
    Director->OnMatchWon(0, false);
    TestEqual(TEXT("events reach the director"), Director->Log,
        FString(TEXT("match; start 1 mine 3; end 1 mine 3; start 0 theirs 4; checkpoint 1 mine None 0 Stage 1; won 0 theirs; ")));
    TestEqual(TEXT("no controller: no faction"), Director->GetFaction(), static_cast<int32>(INDEX_NONE));
    TestFalse(TEXT("...so nothing is its own"), Director->IsMine(INDEX_NONE));

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

    // A match preset names the three director classes
    UConnectIt_MatchPresetDataAsset* Preset = NewObject<UConnectIt_MatchPresetDataAsset>();
    Preset->AIDirectorClass = UConnectIt_TestAIDirector::StaticClass();
    Preset->PlayerDirectorClass = UConnectIt_PlayerDirector::StaticClass();
    Preset->LevelDirectorClass = UConnectIt_LevelDirector::StaticClass();
    TestTrue(TEXT("a preset names its director classes"),
        Preset->AIDirectorClass == UConnectIt_TestAIDirector::StaticClass() && Preset->PlayerDirectorClass && Preset->LevelDirectorClass);
#if WITH_EDITOR
    {
        // The base Player director has no starting loadout: the preset is invalid
        FDataValidationContext Validation;
        TestTrue(TEXT("a player director with no loadout makes the preset invalid"),
            Preset->IsDataValid(Validation) == EDataValidationResult::Invalid);
    }
#endif
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItDirectorConditionsTest,
    "ConnectIt.AI.DirectorConditions", TestFlags)

bool FConnectItDirectorConditionsTest::RunTest(const FString& Parameters)
{
    // Both sides' goals live on the Level director
    UConnectIt_MatchConditionDataAsset* ScoreGoal = NewObject<UConnectIt_MatchConditionDataAsset>(
        GetTransientPackage(), TEXT("WC_Test_DirectorScore"));
    ScoreGoal->Conditions.Add(TInstancedStruct<FConnectItMatchCondition>::Make<FConnectItMatchCondition_ScoreThreshold>(ScoreAtLeast(4.f)));
    UConnectIt_MatchConditionDataAsset* EmptyGoal = NewObject<UConnectIt_MatchConditionDataAsset>(
        GetTransientPackage(), TEXT("WC_Test_DirectorEmpty"));

    UConnectIt_LevelDirector* Level = NewObject<UConnectIt_LevelDirector>();
    Level->PlayerWinConditions = { ScoreGoal, nullptr };
    Level->AIWinConditions = { ScoreGoal };

    // Resolving from the Level director's lists: what the match plays with
    FConnectItRuleSet Rules;
    AddExpectedError(TEXT("Player win condition 1 is empty"), EAutomationExpectedErrorFlags::Contains, 1);
    Rules.ResolveConditionGroups(Level->PlayerWinConditions, Level->AIWinConditions);
    TestEqual(TEXT("the player's goal, the empty entry skipped"), Rules.PlayerWinGroups.Num(), 1);
    TestTrue(TEXT("the AI's goal"), Rules.AIWinGroups.Num() == 1 && Rules.AIWinGroups[0].Source == ScoreGoal);

    // One side's goals replaced: only that list changes
    Rules.SetConditionGroups(/*bForAI=*/true, /*bWinGoals=*/true, { ScoreGoal, ScoreGoal });
    TestEqual(TEXT("the AI's goals replaced"), Rules.GetConditionGroups(true, true).Num(), 2);
    TestEqual(TEXT("...the player's untouched"), Rules.PlayerWinGroups.Num(), 1);
    Rules.SetConditionGroups(false, false, { ScoreGoal });
    TestEqual(TEXT("the player's lose conditions"), Rules.PlayerLoseGroups.Num(), 1);

#if WITH_EDITOR
    {
        // An empty entry is a warning; a goal with no conditions is invalid
        FDataValidationContext Validation;
        Level->IsDataValid(Validation);
        TestEqual(TEXT("level: no errors"), static_cast<int32>(Validation.GetNumErrors()), 0);
        TestTrue(TEXT("level: the empty entry is a warning"), Validation.GetNumWarnings() >= 1);

        FDataValidationContext Broken;
        Level->PlayerWinConditions = { EmptyGoal };
        Level->IsDataValid(Broken);
        TestEqual(TEXT("a goal with no conditions is an error"), static_cast<int32>(Broken.GetNumErrors()), 1);

        FDataValidationContext NoGoals;
        Level->PlayerWinConditions.Reset();
        Level->AIWinConditions.Reset();
        Level->IsDataValid(NoGoals);
        TestEqual(TEXT("no player goals is an error"), static_cast<int32>(NoGoals.GetNumErrors()), 1);
        TestTrue(TEXT("no AI goals is warned about"), NoGoals.GetNumWarnings() >= 1);
    }
#endif
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItBoardLayoutTest,
    "ConnectIt.Board.Layout", TestFlags)

bool FConnectItBoardLayoutTest::RunTest(const FString& Parameters)
{
    // Layouts and what changes between them: tiles that go, tiles that come,
    // shared positions untouched
    const auto MakeLayout = [](std::initializer_list<FGridPosition> Positions)
    {
        UConnectIt_BoardLayoutDataAsset* Layout = NewObject<UConnectIt_BoardLayoutDataAsset>();
        for (const FGridPosition& Position : Positions)
        {
            FConnectItBoardLayoutCell& Cell = Layout->Cells.AddDefaulted_GetRef();
            Cell.Position = Position;
        }
        return Layout;
    };
    const auto Has = [](const TArray<FGridPosition>& Positions, int32 X, int32 Y)
    {
        return Positions.Contains(FGridPosition(X, Y));
    };

    const UConnectIt_BoardLayoutDataAsset* Row = MakeLayout({ {0, 0}, {1, 0}, {2, 0}, {3, 0} });
    const UConnectIt_BoardLayoutDataAsset* Square = MakeLayout({ {0, 0}, {1, 0}, {0, 1}, {1, 1} });

    // Identical: nothing changes
    TestTrue(TEXT("identical: no change"), Row->DiffFrom(*Row).IsEmpty());

    // Grow: a 4x1 row to a 4x4 square -- only the new tiles come
    UConnectIt_BoardLayoutDataAsset* Big = NewObject<UConnectIt_BoardLayoutDataAsset>();
    for (int32 X = 0; X < 4; X++)
    {
        for (int32 Y = 0; Y < 4; Y++)
        {
            Big->Cells.AddDefaulted_GetRef().Position = FGridPosition(X, Y);
        }
    }
    const FConnectItBoardLayoutDiff Grow = Big->DiffFrom(*Row);
    TestEqual(TEXT("grow: nothing goes"), Grow.Removed.Num(), 0);
    TestEqual(TEXT("grow: 12 tiles come"), Grow.Added.Num(), 12);

    // Shrink: back again -- only the extra tiles go
    const FConnectItBoardLayoutDiff Shrink = Row->DiffFrom(*Big);
    TestEqual(TEXT("shrink: 12 tiles go"), Shrink.Removed.Num(), 12);
    TestEqual(TEXT("shrink: nothing comes"), Shrink.Added.Num(), 0);

    // Move: a row to a square sharing two tiles
    const FConnectItBoardLayoutDiff Move = Square->DiffFrom(*Row);
    TestTrue(TEXT("move: the row's far end goes"),
        Move.Removed.Num() == 2 && Has(Move.Removed, 2, 0) && Has(Move.Removed, 3, 0));
    TestTrue(TEXT("move: the square's top comes, in layout order"),
        Move.Added.Num() == 2 && Move.Added[0].Position == FGridPosition(0, 1) && Move.Added[1].Position == FGridPosition(1, 1));

    // From a live board: the board's tiles are the "current" side
    const FConnectItBoardState Board = MakeBoard(2);
    const FConnectItBoardLayoutDiff FromBoard = Row->DiffFrom(Board);
    TestTrue(TEXT("from a board: its top row goes, the row's far end comes"),
        FromBoard.Removed.Num() == 2 && Has(FromBoard.Removed, 0, 1) && Has(FromBoard.Removed, 1, 1)
        && FromBoard.Added.Num() == 2);

    // A cell keeps its data on the way through
    UConnectIt_BoardLayoutDataAsset* Charged = MakeLayout({ {5, 5} });
    Charged->Cells[0].TileData.Multiplier = 3.f;
    Charged->Cells[0].TileData.StartingFactionPiece = 1;
    const FConnectItBoardLayoutDiff Carry = Charged->DiffFrom(Board);
    TestTrue(TEXT("added cells carry their tile data"),
        Carry.Added.Num() == 1 && Carry.Added[0].TileData.Multiplier == 3.f && Carry.Added[0].TileData.StartingFactionPiece == 1);
    TestNotNull(TEXT("find a cell by position"), Charged->FindCell(FGridPosition(5, 5)));
    TestNull(TEXT("...and not one that isn't there"), Charged->FindCell(FGridPosition(0, 0)));

#if WITH_EDITOR
    {
        FDataValidationContext Fine;
        TestTrue(TEXT("a plain layout is valid"), Square->IsDataValid(Fine) != EDataValidationResult::Invalid);

        UConnectIt_BoardLayoutDataAsset* Twice = MakeLayout({ {0, 0}, {0, 0} });
        FDataValidationContext Duplicate;
        TestTrue(TEXT("a repeated position is invalid"), Twice->IsDataValid(Duplicate) == EDataValidationResult::Invalid);
        TestTrue(TEXT("...and the diff keeps one"), Twice->DiffFrom(*Row).Added.IsEmpty() && Twice->DiffFrom(*Row).Removed.Num() == 3);

        UConnectIt_BoardLayoutDataAsset* ThirdSide = MakeLayout({ {0, 0} });
        ThirdSide->Cells[0].TileData.StartingFactionPiece = 2;
        FDataValidationContext Faction;
        ThirdSide->IsDataValid(Faction);
        TestTrue(TEXT("a piece for a faction no match has is warned about"), Faction.GetNumWarnings() >= 1);
    }
#endif
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItTileOperationsTest,
    "ConnectIt.Board.TileOperations", TestFlags)

bool FConnectItTileOperationsTest::RunTest(const FString& Parameters)
{
    const auto MakeCell = [](int32 X, int32 Y, int32 StartingFaction = -1, float Multiplier = 1.f)
    {
        FConnectItBoardLayoutCell Cell;
        Cell.Position = FGridPosition(X, Y);
        Cell.TileData.StartingFactionPiece = StartingFaction;
        Cell.TileData.Multiplier = Multiplier;
        return Cell;
    };
    const auto EventAt = [](const FConnectItBoardChangeEvent& Events, int32 Index) -> const FInstancedStruct*
    {
        return Events.Events.IsValidIndex(Index) ? &Events.Events[Index] : nullptr;
    };

    // --- Remove Tiles ---
    {
        FConnectItBoardState Board = MakeBoard(3);
        Place(Board, 1, 1, 0);
        FConnectItBoardOperation_RemoveTiles Remove;
        Remove.Positions = { FGridPosition(1, 1), FGridPosition(2, 2) };
        TestTrue(TEXT("remove: can apply"), Remove.CanApply(Board));

        FConnectItBoardChanges Touched;
        FConnectItBoardChangeEvent Events;
        Remove.Apply(Board, Touched, &Events);
        TestEqual(TEXT("remove: two tiles fewer"), Board.NumTiles(), 7);
        TestNull(TEXT("remove: the tile is gone"), Board.GetTileData(FGridPosition(1, 1)));
        TestTrue(TEXT("remove: tiles disappearing set off nothing"), Touched.IsEmpty());
        const FConnectItBoardEvent_TilesRemoved* Removed =
            EventAt(Events, 0) ? EventAt(Events, 0)->GetPtr<FConnectItBoardEvent_TilesRemoved>() : nullptr;
        TestTrue(TEXT("remove: the event says which, and the piece that went"),
            Removed && Removed->Positions.Num() == 2 && Removed->PieceFactions == TArray<int32>({ 0, -1 }));

        FConnectItBoardOperation_RemoveTiles Bad;
        TestFalse(TEXT("remove: nothing to remove is refused"), Bad.CanApply(Board));
        Bad.Positions = { FGridPosition(1, 1) };
        TestFalse(TEXT("remove: a position that isn't a tile is refused"), Bad.CanApply(Board));
        Bad.Positions = { FGridPosition(0, 0), FGridPosition(0, 0) };
        TestFalse(TEXT("remove: a repeated position is refused"), Bad.CanApply(Board));
    }

    // --- Add Tiles: an added starting piece is an arrival and can score ---
    {
        FConnectItRuleSet Rules;   // Lines of 4
        FConnectItBoardState Board = MakeBoard(3);
        Board.StartingPieces = { 0, 0 };
        Place(Board, 0, 0, 0);
        Place(Board, 1, 0, 0);
        Place(Board, 2, 0, 0);

        FConnectItBoardOperation_AddTiles Add;
        Add.Faction = INDEX_NONE;
        Add.Cells = { MakeCell(3, 0, 0, 2.f), MakeCell(3, 1) };
        TestTrue(TEXT("add: can apply"), Add.CanApply(Board));

        FConnectItBoardChangeEvent Events;
        const float Points = Rules.CarryOutOperation(Add, Board, &Events);
        TestEqual(TEXT("add: two tiles more"), Board.NumTiles(), 11);
        TestTrue(TEXT("add: the starting piece completed a line of 4"), Points > 0.f);
        TestTrue(TEXT("add: and stays, like a placed piece"), Board.IsTileOccupied(FGridPosition(3, 0)));
        TestEqual(TEXT("add: it counts as a starting piece"), Board.StartingPieces[0], 1);
        const FConnectItBoardEvent_TilesAdded* Added =
            EventAt(Events, 0) ? EventAt(Events, 0)->GetPtr<FConnectItBoardEvent_TilesAdded>() : nullptr;
        TestTrue(TEXT("add: Tiles Added first, carrying the cells"), Added && Added->Cells.Num() == 2);
        TestTrue(TEXT("add: then Scored"),
            EventAt(Events, 1) && EventAt(Events, 1)->GetPtr<FConnectItBoardEvent_Scored>() != nullptr);

        FConnectItBoardOperation_AddTiles Bad;
        TestFalse(TEXT("add: nothing to add is refused"), Bad.CanApply(Board));
        Bad.Cells = { MakeCell(0, 0) };
        TestFalse(TEXT("add: a position that is already a tile is refused"), Bad.CanApply(Board));
        Bad.Cells = { MakeCell(9, 9), MakeCell(9, 9) };
        TestFalse(TEXT("add: a repeated position is refused"), Bad.CanApply(Board));
    }

    // --- Change Layout: one change, one win check on the final board ---
    {
        // The player loses with no pieces left. The in-between board (old
        // tiles gone, new ones not yet added) has none; the final board has one.
        FConnectItRuleSet Rules;
        Rules.AIFaction = 1;
        AddCondition(AddGroup(Rules.PlayerWinGroups), ScoreAtLeast(100.f));
        AddCondition(AddGroup(Rules.PlayerLoseGroups), PiecesIs(EConnectItPieceCountComparison::Equal, 0));

        FConnectItBoardState Board = MakeBoard(2);
        Place(Board, 0, 0, 0);
        Board.GetTileDataMutable(FGridPosition(1, 1))->Multiplier = 3.f;
        Place(Board, 1, 1, 1);

        FConnectItBoardOperation_ChangeLayout Change;
        Change.RemovePositions = { FGridPosition(0, 0), FGridPosition(0, 1) };
        Change.AddCells = { MakeCell(5, 5, 0), MakeCell(0, 0) };
        Change.LayoutName = TEXT("CIBL_Test");
        Change.bHasCamera = true;
        TestTrue(TEXT("layout: a position removed and added again is fine"), Change.CanApply(Board));

        FConnectItBoardChangeEvent Events;
        Rules.CarryOutOperation(Change, Board, &Events);
        Rules.StampWinState(Board, &Events);
        TestFalse(TEXT("layout: no loss from the in-between board"), Board.bGameOver);
        TestTrue(TEXT("layout: shared tiles keep piece and multiplier"),
            Board.IsTileOccupied(FGridPosition(1, 1)) && Board.GetTileData(FGridPosition(1, 1))->Multiplier == 3.f);
        TestTrue(TEXT("layout: (0,0) is a fresh, empty tile"), Board.GetTileData(FGridPosition(0, 0)) && !Board.IsTileOccupied(FGridPosition(0, 0)));
        TestEqual(TEXT("layout: tile count"), Board.NumTiles(), 4);

        TestTrue(TEXT("layout: Tiles Removed, Tiles Added, Layout Changed, in that order"),
            EventAt(Events, 0) && EventAt(Events, 0)->GetPtr<FConnectItBoardEvent_TilesRemoved>()
            && EventAt(Events, 1) && EventAt(Events, 1)->GetPtr<FConnectItBoardEvent_TilesAdded>()
            && EventAt(Events, 2) && EventAt(Events, 2)->GetPtr<FConnectItBoardEvent_LayoutChanged>());
        const FConnectItBoardEvent_LayoutChanged* Changed =
            EventAt(Events, 2) ? EventAt(Events, 2)->GetPtr<FConnectItBoardEvent_LayoutChanged>() : nullptr;
        TestTrue(TEXT("layout: the event names the layout and its camera"),
            Changed && Changed->LayoutName == FName(TEXT("CIBL_Test")) && Changed->bHasCamera);

        // From a diff; an identical layout changes no tile but still says so
        FConnectItBoardOperation_ChangeLayout Same;
        Same.SetDiff(FConnectItBoardLayoutDiff::Between(Board.TilePositions, {}));
        TestEqual(TEXT("layout: a diff to nothing removes every tile"), Same.RemovePositions.Num(), 4);
        FConnectItBoardOperation_ChangeLayout Nothing;
        TestTrue(TEXT("layout: an empty change is allowed (camera only)"), Nothing.CanApply(Board));

        FConnectItBoardOperation_ChangeLayout Bad;
        Bad.AddCells = { MakeCell(1, 1) };
        TestFalse(TEXT("layout: adding over a tile that stays is refused"), Bad.CanApply(Board));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItPhasesAndLayoutsTest,
    "ConnectIt.Board.PhasesAndLayouts", TestFlags)

bool FConnectItPhasesAndLayoutsTest::RunTest(const FString& Parameters)
{
    // --- Begin Phase: the board records the phase and says so ---
    const FGameplayTag Phase1 = FGameplayTag::RequestGameplayTag(TEXT("ConnectIt.Phase.1"), false);
    const FGameplayTag Phase2 = FGameplayTag::RequestGameplayTag(TEXT("ConnectIt.Phase.2"), false);
    if (!TestTrue(TEXT("the phase tags exist (DefaultGameplayTags.ini)"), Phase1.IsValid() && Phase2.IsValid()))
    {
        return false;
    }

    FConnectItRuleSet Rules;
    FConnectItBoardState Board = MakeBoard(3);
    TestFalse(TEXT("no phase before one begins"), Board.CurrentPhase.IsValid());

    FConnectItBoardOperation_BeginPhase Begin;
    Begin.Phase = Phase1;
    TestTrue(TEXT("begin: can apply"), Begin.CanApply(Board));
    FConnectItBoardChangeEvent First;
    Rules.CarryOutOperation(Begin, Board, &First);
    TestTrue(TEXT("begin: the board is in phase 1"), Board.CurrentPhase == Phase1);
    const FConnectItBoardEvent_PhaseChanged* FirstEvent =
        First.Events.Num() == 1 ? First.Events[0].GetPtr<FConnectItBoardEvent_PhaseChanged>() : nullptr;
    TestTrue(TEXT("begin: Phase Changed, from no phase"), FirstEvent && FirstEvent->Phase == Phase1 && !FirstEvent->PreviousPhase.IsValid());

    Begin.Phase = Phase2;
    FConnectItBoardChangeEvent Second;
    Rules.CarryOutOperation(Begin, Board, &Second);
    const FConnectItBoardEvent_PhaseChanged* SecondEvent =
        Second.Events.Num() == 1 ? Second.Events[0].GetPtr<FConnectItBoardEvent_PhaseChanged>() : nullptr;
    TestTrue(TEXT("begin: the next phase names the one before"), SecondEvent && SecondEvent->PreviousPhase == Phase1 && SecondEvent->Phase == Phase2);

    FConnectItBoardOperation_BeginPhase NoPhase;
    TestFalse(TEXT("begin: no phase is refused"), NoPhase.CanApply(Board));

    // --- Set Layout's change: worked out against the board it will change ---
    UConnectIt_BoardLayoutDataAsset* Layout = NewObject<UConnectIt_BoardLayoutDataAsset>(GetTransientPackage(), TEXT("CIBL_TestRow"));
    for (int32 X = 0; X < 4; X++)
    {
        Layout->Cells.AddDefaulted_GetRef().Position = FGridPosition(X, 0);
    }
    Layout->bHasCamera = true;
    Layout->CameraTransform.SetLocation(FVector(1.f, 2.f, 3.f));

    const FConnectItBoardOperation_ChangeLayout FromNow = Layout->MakeChangeFrom(Board);
    TestEqual(TEXT("layout: the 3x3 board's rows 1-2 go"), FromNow.RemovePositions.Num(), 6);
    TestEqual(TEXT("layout: (3,0) comes"), FromNow.AddCells.Num(), 1);
    TestTrue(TEXT("layout: it carries the layout's name and camera"),
        FromNow.LayoutName == FName(TEXT("CIBL_TestRow")) && FromNow.bHasCamera
        && FromNow.CameraTransform.GetLocation().Equals(FVector(1.f, 2.f, 3.f)));

    // The board changes before the layout is applied (another change queued
    // ahead of it): the layout's change is worked out against the later board
    FConnectItBoardOperation_RemoveTiles Earlier;
    Earlier.Positions = { FGridPosition(0, 2), FGridPosition(1, 2), FGridPosition(2, 2) };
    FConnectItBoardChanges Touched;
    Earlier.Apply(Board, Touched, nullptr);
    const FConnectItBoardOperation_ChangeLayout Later = Layout->MakeChangeFrom(Board);
    TestEqual(TEXT("layout: against the later board, only row 1 goes"), Later.RemovePositions.Num(), 3);
    TestTrue(TEXT("layout: and it applies"), Later.CanApply(Board));

    Rules.CarryOutOperation(Later, Board, nullptr);
    TestEqual(TEXT("layout: the board is the layout"), Board.NumTiles(), 4);
    TestTrue(TEXT("layout: every cell is a tile"), Layout->DiffFrom(Board).IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItChangeGoalsTest,
    "ConnectIt.Board.ChangeGoals", TestFlags)

bool FConnectItChangeGoalsTest::RunTest(const FString& Parameters)
{
    // The board's side of a goals change: progress restarts, score stays,
    // and the win check that follows measures the new goals
    UConnectIt_MatchConditionDataAsset* Score4 = NewObject<UConnectIt_MatchConditionDataAsset>(GetTransientPackage(), TEXT("WC_Test_Goals4"));
    Score4->Conditions.Add(TInstancedStruct<FConnectItMatchCondition>::Make<FConnectItMatchCondition_ScoreThreshold>(ScoreAtLeast(4.f)));
    UConnectIt_MatchConditionDataAsset* Score20 = NewObject<UConnectIt_MatchConditionDataAsset>(GetTransientPackage(), TEXT("WC_Test_Goals20"));
    Score20->Conditions.Add(TInstancedStruct<FConnectItMatchCondition>::Make<FConnectItMatchCondition_ScoreThreshold>(ScoreAtLeast(20.f)));

    FConnectItRuleSet Rules;
    Rules.AIFaction = 1;
    Rules.ResolveConditionGroups({ Score20, Score20 }, { Score20 });

    FConnectItBoardState Board = MakeBoard(5);
    Board.ScoreBoard = { 6.f, 0.f };
    Board.GroupsCompleted = { 1, 0 };

    // New player goals: 20 points (not met), then 4 points
    Rules.SetConditionGroups(false, true, { Score20, Score4 });
    FConnectItBoardOperation_ChangeGoals Change;
    Change.Factions = { 0 };
    Change.bWinGoals = true;
    Change.GroupNames = { Score20->GetFName(), Score4->GetFName() };
    TestTrue(TEXT("can apply"), Change.CanApply(Board));

    FConnectItBoardChangeEvent Events;
    const float Points = Rules.CarryOutOperation(Change, Board, &Events);
    Rules.StampWinState(Board, &Events);
    TestEqual(TEXT("nothing scores"), Points, 0.f);
    TestEqual(TEXT("score carries over"), Board.ScoreBoard[0], 6.f);
    TestEqual(TEXT("progress restarted (the first new goal isn't met)"), Board.GroupsCompleted[0], 0);
    TestFalse(TEXT("no win yet"), Board.bGameOver);
    const FConnectItBoardEvent_GoalsChanged* Changed =
        Events.Events.Num() > 0 ? Events.Events[0].GetPtr<FConnectItBoardEvent_GoalsChanged>() : nullptr;
    TestTrue(TEXT("Goals Changed comes first, with the new goals"),
        Changed && Changed->Factions == TArray<int32>{ 0 } && Changed->bWinGoals && Changed->GroupNames.Num() == 2);

    // New goals already met: they complete at once (with the win)
    Rules.SetConditionGroups(false, true, { Score4 });
    Change.GroupNames = { Score4->GetFName() };
    FConnectItBoardChangeEvent MetEvents;
    Rules.CarryOutOperation(Change, Board, &MetEvents);
    Rules.StampWinState(Board, &MetEvents);
    TestTrue(TEXT("a goal already met wins at once"), Board.bGameOver && Board.WinningFactionSlot == 0);

    // Lose conditions keep no progress on the board
    FConnectItBoardState Other = MakeBoard(5);
    Other.ScoreBoard = { 0.f, 0.f };
    Other.GroupsCompleted = { 0, 1 };
    FConnectItBoardOperation_ChangeGoals Lose;
    Lose.Factions = { 1 };
    Lose.bWinGoals = false;
    FConnectItBoardChanges Touched;
    Lose.Apply(Other, Touched, nullptr);
    TestEqual(TEXT("lose conditions don't touch win progress"), Other.GroupsCompleted[1], 1);

    // Refusals
    FConnectItBoardOperation_ChangeGoals Nobody;
    TestFalse(TEXT("no factions: refused"), Nobody.CanApply(Board));
    Nobody.Factions = { 5 };
    TestFalse(TEXT("a faction not on the board: refused"), Nobody.CanApply(Board));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItLevelTriggersTest,
    "ConnectIt.AI.LevelTriggers", TestFlags)

bool FConnectItLevelTriggersTest::RunTest(const FString& Parameters)
{
    UConnectIt_MatchConditionDataAsset* Score4 = NewObject<UConnectIt_MatchConditionDataAsset>(GetTransientPackage(), TEXT("WC_Test_Trigger4"));
    Score4->Conditions.Add(TInstancedStruct<FConnectItMatchCondition>::Make<FConnectItMatchCondition_ScoreThreshold>(ScoreAtLeast(4.f)));
    UConnectIt_MatchConditionDataAsset* Score8 = NewObject<UConnectIt_MatchConditionDataAsset>(GetTransientPackage(), TEXT("WC_Test_Trigger8"));
    Score8->Conditions.Add(TInstancedStruct<FConnectItMatchCondition>::Make<FConnectItMatchCondition_ScoreThreshold>(ScoreAtLeast(8.f)));

    FConnectItRuleSet Rules;
    Rules.AIFaction = 1;

    UConnectIt_TestLevelDirector* Level = NewObject<UConnectIt_TestLevelDirector>();
    Level->Triggers.Add({ Score4, EConnectItTriggerSide::Player });
    Level->Triggers.Add({ Score8, EConnectItTriggerSide::AI });

    FConnectItBoardState Board = MakeBoard(5);
    Board.ScoreBoard = { 0.f, 0.f };
    Level->CheckTriggers(Board, Rules);
    TestEqual(TEXT("nothing met, nothing fired"), Level->Log, FString());
    TestTrue(TEXT("armed"), Level->IsTriggerArmed(Score4));

    // The player reaches 4: its trigger fires once
    Board.ScoreBoard = { 4.f, 0.f };
    Level->CheckTriggers(Board, Rules);
    Level->CheckTriggers(Board, Rules);
    TestEqual(TEXT("fires once, for the player"), Level->Log, FString(TEXT("WC_Test_Trigger4 0; ")));
    TestFalse(TEXT("then disarmed"), Level->IsTriggerArmed(Score4));

    // The AI trigger ignores the player's score; fires on the AI's
    Board.ScoreBoard = { 9.f, 0.f };
    Level->CheckTriggers(Board, Rules);
    TestEqual(TEXT("an AI trigger ignores the player"), Level->Log, FString(TEXT("WC_Test_Trigger4 0; ")));
    Board.ScoreBoard = { 9.f, 8.f };
    Level->CheckTriggers(Board, Rules);
    TestEqual(TEXT("...and fires for the AI"), Level->Log, FString(TEXT("WC_Test_Trigger4 0; WC_Test_Trigger8 1; ")));

    // Re-armed, it fires again the next time it holds
    Level->RearmTrigger(Score4);
    TestTrue(TEXT("re-armed"), Level->IsTriggerArmed(Score4));
    Level->Log.Reset();
    Level->CheckTriggers(Board, Rules);
    TestEqual(TEXT("fires again after re-arming"), Level->Log, FString(TEXT("WC_Test_Trigger4 0; ")));

    // Re-armed from inside its own reaction: fires again on the next check
    Level->RearmOnFire = Score4;
    Level->RearmTrigger(Score4);
    Level->Log.Reset();
    Level->CheckTriggers(Board, Rules);
    Level->CheckTriggers(Board, Rules);
    TestEqual(TEXT("a reaction can re-arm its own trigger"), Level->Log, FString(TEXT("WC_Test_Trigger4 0; WC_Test_Trigger4 0; ")));

    // Nothing after the game is over
    Level->Log.Reset();
    Board.bGameOver = true;
    Level->CheckTriggers(Board, Rules);
    TestEqual(TEXT("not after the game is over"), Level->Log, FString());

    // Either side: the lowest faction that meets it
    UConnectIt_TestLevelDirector* Any = NewObject<UConnectIt_TestLevelDirector>();
    Any->Triggers.Add({ Score4, EConnectItTriggerSide::Either });
    FConnectItBoardState Both = MakeBoard(5);
    Both.ScoreBoard = { 0.f, 5.f };
    Any->CheckTriggers(Both, Rules);
    TestEqual(TEXT("either side: the AI's score counts"), Any->Log, FString(TEXT("WC_Test_Trigger4 1; ")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItLevelChangesTest,
    "ConnectIt.AI.LevelChanges", TestFlags)

bool FConnectItLevelChangesTest::RunTest(const FString& Parameters)
{
    // When a level change applies: now, once the board change resolving has
    // played out, or at the next turn's start -- always in the order asked
    FConnectItLevelChangeQueue Queue;
    const auto MakeChange = [](int32 AsFaction)
    {
        FConnectItLevelChange Change;
        Change.Operation = FInstancedStruct::Make(FConnectItBoardOperation_ToggleTileActive());
        Change.AsFaction = AsFaction;
        return Change;
    };

    TestTrue(TEXT("immediately with nothing resolving: apply now"),
        Queue.Add(MakeChange(0), EConnectItChangeTiming::Immediately, false));
    TestEqual(TEXT("...nothing waiting"), Queue.NumWaiting(), 0);

    TestFalse(TEXT("immediately while resolving: wait"), Queue.Add(MakeChange(1), EConnectItChangeTiming::Immediately, true));
    TestFalse(TEXT("next turn: wait for it"), Queue.Add(MakeChange(2), EConnectItChangeTiming::NextTurnStart, false));
    TestFalse(TEXT("nothing resolving, but one waiting: behind it"),
        Queue.Add(MakeChange(3), EConnectItChangeTiming::Immediately, false));
    TestEqual(TEXT("two waiting"), Queue.NumWaiting(), 2);
    TestEqual(TEXT("one for the next turn"), Queue.NumForNextTurn(), 1);

    FConnectItLevelChange Taken;
    TestTrue(TEXT("first waiting..."), Queue.TakeNext(Taken) && Taken.AsFaction == 1);

    Queue.StartTurn();
    TestEqual(TEXT("the turn's change joins the line"), Queue.NumWaiting(), 2);
    TestEqual(TEXT("...none left for the next turn"), Queue.NumForNextTurn(), 0);
    TestTrue(TEXT("...behind the one already waiting"), Queue.TakeNext(Taken) && Taken.AsFaction == 3);
    TestTrue(TEXT("...then the turn's"), Queue.TakeNext(Taken) && Taken.AsFaction == 2);
    TestFalse(TEXT("all taken"), Queue.TakeNext(Taken));

    Queue.Add(MakeChange(4), EConnectItChangeTiming::Immediately, true);
    Queue.Add(MakeChange(5), EConnectItChangeTiming::Immediately, true);
    Queue.Add(MakeChange(6), EConnectItChangeTiming::NextTurnStart, true);
    TestEqual(TEXT("discarding drops the waiting ones"), Queue.DiscardWaiting(), 2);
    TestEqual(TEXT("...but not the next turn's"), Queue.NumForNextTurn(), 1);

    // The scoring rule a level sets
    FConnectItRuleSet Rules;
    FConnectItScoringRule_Lines Five;
    Five.ConnectLength = 5;
    TestTrue(TEXT("a scoring rule is accepted"), Rules.SetScoringRuleFrom(FInstancedStruct::Make(Five)));
    TestEqual(TEXT("...and used"), Rules.GetScoringRule()->GetMinPiecesToScore(), 5);
    TestFalse(TEXT("a board operation is not a scoring rule"),
        Rules.SetScoringRuleFrom(FInstancedStruct::Make(FConnectItBoardOperation_PlacePiece())));
    TestEqual(TEXT("...and changes nothing"), Rules.GetScoringRule()->GetMinPiecesToScore(), 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItDirectorOrderTest,
    "ConnectIt.AI.DirectorOrder", TestFlags)

bool FConnectItDirectorOrderTest::RunTest(const FString& Parameters)
{
    // Events go to the Level director, then the Player directors (by slot;
    // these have no player yet, so they keep their order), then the AI
    UConnectIt_LevelDirector* Level = NewObject<UConnectIt_LevelDirector>();
    UConnectIt_PlayerDirector* PlayerA = NewObject<UConnectIt_PlayerDirector>();
    UConnectIt_PlayerDirector* PlayerB = NewObject<UConnectIt_PlayerDirector>();
    UConnectIt_TestAIDirector* AI = NewObject<UConnectIt_TestAIDirector>();

    const TArray<UConnectIt_Director*> Ordered = UConnectIt_Director::InDispatchOrder(Level, { PlayerA, PlayerB }, AI);
    if (!TestEqual(TEXT("all four"), Ordered.Num(), 4)) return false;
    TestTrue(TEXT("level first"), Ordered[0] == Level);
    TestTrue(TEXT("then the players"), Ordered[1] == PlayerA && Ordered[2] == PlayerB);
    TestTrue(TEXT("AI last"), Ordered[3] == AI);

    const TArray<UConnectIt_Director*> NoLevel = UConnectIt_Director::InDispatchOrder(nullptr, { PlayerA }, nullptr);
    TestTrue(TEXT("missing directors are skipped"), NoLevel.Num() == 1 && NoLevel[0] == PlayerA);

    TestEqual(TEXT("the level director speaks for no side"), Level->GetFaction(), static_cast<int32>(INDEX_NONE));
    TestEqual(TEXT("a player director with no player has no side yet"), PlayerA->GetFaction(), static_cast<int32>(INDEX_NONE));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItEventsReplayTest,
    "ConnectIt.Board.Events.Replay", TestFlags)

bool FConnectItEventsReplayTest::RunTest(const FString& Parameters)
{
    // The display board's promise: the board before a change, plus every one
    // of its events applied in order, is the board after it (tiles and
    // scores). Carries Operation out on a copy of Before and replays its
    // events on another; returns the events.
    const auto Replay = [this](const TCHAR* What, const FConnectItRuleSet& Rules,
        const FConnectItBoardOperation& Operation, const FConnectItBoardState& Before,
        FConnectItBoardState* OutAfter = nullptr, FConnectItBoardState* OutReplayed = nullptr)
    {
        FConnectItBoardChangeEvent Events;
        FConnectItBoardState After = Before;
        FString WhyNot;
        const bool bCanApply = Operation.CanApply(After, &WhyNot);
        if (!TestTrue(FString::Printf(TEXT("%s: can apply (%s)"), What, *WhyNot), bCanApply))
        {
            return Events;
        }
        Rules.CarryOutOperation(Operation, After, &Events);

        FConnectItBoardState Replayed = Before;
        FConnectItBoardEvent::ApplyAll(Events, Replayed);
        TestTrue(FString::Printf(TEXT("%s: before + its events = after"), What), Replayed.HasSameTilesAndScores(After));
        if (OutAfter) *OutAfter = After;
        if (OutReplayed) *OutReplayed = Replayed;
        return Events;
    };
    const auto MakeCell = [](int32 X, int32 Y, int32 StartingFaction = -1, float Multiplier = 1.f)
    {
        FConnectItBoardLayoutCell Cell;
        Cell.Position = FGridPosition(X, Y);
        Cell.TileData.StartingFactionPiece = StartingFaction;
        Cell.TileData.Multiplier = Multiplier;
        return Cell;
    };

    FConnectItRuleSet Rules;   // Lines of 4

    // --- A placement scoring two crossing lines ---
    {
        FConnectItBoardState Board = MakeBoard(5);
        Place(Board, 0, 1, 0);
        Place(Board, 1, 1, 0);
        Place(Board, 2, 1, 0);
        Place(Board, 3, 2, 0);
        Place(Board, 3, 3, 0);
        Place(Board, 3, 4, 0);
        Board.GetTileDataMutable(FGridPosition(1, 1))->Multiplier = 2.f;

        const FConnectItBoardChangeEvent Events =
            Replay(TEXT("place, two lines"), Rules, MakeAt<FConnectItBoardOperation_PlacePiece>(0, 3, 1), Board);
        const auto* First = EventAt<FConnectItBoardEvent_Scored>(Events, 1);
        const auto* Second = EventAt<FConnectItBoardEvent_Scored>(Events, 2);
        if (TestTrue(TEXT("place: two Scored events"), First && Second))
        {
            TestEqual(TEXT("scored: one new multiplier per position"), First->NewMultipliers.Num(), First->Positions.Num());
            const auto* RowLine = First->Positions.Contains(FGridPosition(1, 1)) ? First : Second;
            const int32 Raised = RowLine->Positions.IndexOfByKey(FGridPosition(1, 1));
            if (TestTrue(TEXT("scored: the x2 tile is in a line"), Raised != INDEX_NONE))
            {
                TestEqual(TEXT("scored: its new multiplier is 3"), RowLine->NewMultipliers[Raised], 3.f);
            }

            const int32 SharedFirst = First->Positions.IndexOfByKey(FGridPosition(3, 1));
            const int32 SharedSecond = Second->Positions.IndexOfByKey(FGridPosition(3, 1));
            if (TestTrue(TEXT("scored: the shared tile is in both lines"),
                SharedFirst != INDEX_NONE && SharedSecond != INDEX_NONE))
            {
                TestEqual(TEXT("scored: the shared tile after the first line"), First->NewMultipliers[SharedFirst], 2.f);
                TestEqual(TEXT("scored: and after the second"), Second->NewMultipliers[SharedSecond], 3.f);
            }
        }
    }

    // --- Swap completing a line ---
    {
        FConnectItBoardState Board = MakeBoard(5);
        Place(Board, 0, 0, 0);
        Place(Board, 1, 0, 0);
        Place(Board, 2, 0, 0);
        Place(Board, 3, 0, 1);
        Place(Board, 3, 2, 0);
        FConnectItBoardOperation_SwapPieces Swap;
        Swap.Faction = 0;
        Swap.PositionA = FGridPosition(3, 2);
        Swap.PositionB = FGridPosition(3, 0);
        const FConnectItBoardChangeEvent Events = Replay(TEXT("swap"), Rules, Swap, Board);
        TestNotNull(TEXT("swap: it scored"), EventAt<FConnectItBoardEvent_Scored>(Events, 1));
    }

    // --- Shift: whole tiles move, past an unshiftable one ---
    {
        FConnectItBoardState Board = MakeBoard(5);
        Place(Board, 0, 0, 0);
        Place(Board, 1, 0, 1);
        Board.GetTileDataMutable(FGridPosition(2, 0))->bCanShift = false;
        Board.GetTileDataMutable(FGridPosition(4, 0))->Multiplier = 3.f;
        Board.GetTileDataMutable(FGridPosition(3, 0))->bIsActive = false;

        FConnectItBoardOperation_Shift Shift;
        Shift.Faction = 0;
        Shift.Direction = EGridDirection::Right;
        for (int32 X = 0; X < 5; X++)
        {
            Shift.Positions.Add(FGridPosition(X, 0));
        }
        Replay(TEXT("shift"), Rules, Shift, Board);
        Shift.Steps = 2;
        Replay(TEXT("shift two steps"), Rules, Shift, Board);
    }

    // --- The single-tile operations ---
    {
        FConnectItBoardState Board = MakeBoard(5);
        Place(Board, 1, 1, 1);
        Place(Board, 2, 2, 0);
        Board.GetTileDataMutable(FGridPosition(3, 3))->Multiplier = 3.f;

        Replay(TEXT("force place"), Rules, MakeAt<FConnectItBoardOperation_ForcePlacePiece>(0, 1, 1), Board);
        Replay(TEXT("capture"), Rules, MakeAt<FConnectItBoardOperation_CapturePiece>(0, 1, 1), Board);
        Replay(TEXT("remove piece"), Rules, MakeAt<FConnectItBoardOperation_RemovePiece>(0, 1, 1), Board);
        Replay(TEXT("toggle"), Rules, MakeAt<FConnectItBoardOperation_ToggleTileActive>(0, 1, 1), Board);

        const FConnectItBoardChangeEvent Destroyed =
            Replay(TEXT("destroy multiplier"), Rules, MakeAt<FConnectItBoardOperation_DestroyTileMultiplier>(0, 3, 3), Board);
        const auto* Event = OnlyEvent<FConnectItBoardEvent_TileMultiplierDestroyed>(Destroyed);
        TestTrue(TEXT("destroy: the event carries the multiplier it took"), Event && Event->PreviousMultiplier == 3.f);
    }

    // --- A capture completing a line ---
    {
        FConnectItBoardState Board = MakeBoard(5);
        Place(Board, 0, 0, 0);
        Place(Board, 1, 0, 0);
        Place(Board, 2, 0, 0);
        Place(Board, 3, 0, 1);
        const FConnectItBoardChangeEvent Events =
            Replay(TEXT("capture, scoring"), Rules, MakeAt<FConnectItBoardOperation_CapturePiece>(0, 3, 0), Board);
        TestNotNull(TEXT("capture: it scored"), EventAt<FConnectItBoardEvent_Scored>(Events, 1));
    }

    // --- Tiles coming and going ---
    {
        FConnectItBoardState Board = MakeBoard(3);
        Board.StartingPieces = { 0, 0 };
        Place(Board, 0, 0, 0);
        Place(Board, 1, 0, 0);
        Place(Board, 2, 0, 0);

        FConnectItBoardOperation_RemoveTiles Remove;
        Remove.Positions = { FGridPosition(1, 1), FGridPosition(2, 2) };
        Replay(TEXT("remove tiles"), Rules, Remove, Board);

        FConnectItBoardOperation_AddTiles Add;
        Add.Faction = INDEX_NONE;
        Add.Cells = { MakeCell(3, 0, 0, 2.f), MakeCell(3, 1) };
        FConnectItBoardState After;
        FConnectItBoardState Replayed;
        Replay(TEXT("add tiles, scoring"), Rules, Add, Board, &After, &Replayed);
        TestTrue(TEXT("add tiles: the starting piece is counted on the replayed board too"),
            Replayed.StartingPieces == After.StartingPieces);

        UConnectIt_BoardLayoutDataAsset* Layout = NewObject<UConnectIt_BoardLayoutDataAsset>(GetTransientPackage(), TEXT("CIBL_TestReplay"));
        for (int32 X = 0; X < 4; X++)
        {
            FConnectItBoardLayoutCell& Cell = Layout->Cells.AddDefaulted_GetRef();
            Cell.Position = FGridPosition(X, 1);
            Cell.TileData.Multiplier = 1.f + X;
        }
        Replay(TEXT("change layout"), Rules, Layout->MakeChangeFrom(Board), Board);
    }

    // --- Phases ---
    {
        const FGameplayTag Phase1 = FGameplayTag::RequestGameplayTag(TEXT("ConnectIt.Phase.1"), false);
        FConnectItBoardOperation_BeginPhase Begin;
        Begin.Phase = Phase1;
        FConnectItBoardState After;
        FConnectItBoardState Replayed;
        Replay(TEXT("begin phase"), Rules, Begin, MakeBoard(3), &After, &Replayed);
        TestTrue(TEXT("begin phase: the replayed board is in the phase"), Replayed.CurrentPhase == After.CurrentPhase);
    }

    // --- Goals: progress restarts, then a goal already met is a checkpoint ---
    // (The win itself -- game over, winner -- is match-level: the Mediator's
    // Game Won event carries it, and the display board copies it from Current
    // once the queue drains.)
    {
        UConnectIt_MatchConditionDataAsset* Score4 = NewObject<UConnectIt_MatchConditionDataAsset>(GetTransientPackage(), TEXT("WC_Test_Replay4"));
        Score4->Conditions.Add(TInstancedStruct<FConnectItMatchCondition>::Make<FConnectItMatchCondition_ScoreThreshold>(ScoreAtLeast(4.f)));
        UConnectIt_MatchConditionDataAsset* Score20 = NewObject<UConnectIt_MatchConditionDataAsset>(GetTransientPackage(), TEXT("WC_Test_Replay20"));
        Score20->Conditions.Add(TInstancedStruct<FConnectItMatchCondition>::Make<FConnectItMatchCondition_ScoreThreshold>(ScoreAtLeast(20.f)));

        FConnectItRuleSet GoalRules;
        GoalRules.AIFaction = 1;
        GoalRules.ResolveConditionGroups({ Score20, Score20 }, { Score20 });
        GoalRules.SetConditionGroups(false, true, { Score4, Score20 });

        FConnectItBoardState Board = MakeBoard(5);
        Board.ScoreBoard = { 6.f, 0.f };
        Board.GroupsCompleted = { 1, 0 };

        FConnectItBoardOperation_ChangeGoals Change;
        Change.Factions = { 0 };
        Change.bWinGoals = true;
        Change.GroupNames = { Score4->GetFName(), Score20->GetFName() };

        FConnectItBoardChangeEvent Events;
        FConnectItBoardState After = Board;
        GoalRules.CarryOutOperation(Change, After, &Events);
        GoalRules.StampWinState(After, &Events);

        FConnectItBoardState Replayed = Board;
        FConnectItBoardEvent::ApplyAll(Events, Replayed);
        TestTrue(TEXT("goals: before + its events = after"), Replayed.HasSameTilesAndScores(After));
        TestTrue(TEXT("goals: the first new goal is met (a checkpoint, no win)"),
            After.GroupsCompleted[0] == 1 && !After.bGameOver);
        TestTrue(TEXT("goals: the replayed board has the same progress"), Replayed.GroupsCompleted == After.GroupsCompleted);
    }

    // --- The check itself notices a difference ---
    {
        const FConnectItBoardState Board = MakeBoard(3);
        FConnectItBoardState Moved = Board;
        Place(Moved, 1, 1, 0);
        TestFalse(TEXT("a different piece is a different board"), Moved.HasSameTilesAndScores(Board));
        FConnectItBoardState Scored = Board;
        Scored.ScoreBoard[1] = 2.f;
        TestFalse(TEXT("a different score is a different board"), Scored.HasSameTilesAndScores(Board));
        FConnectItBoardState Reordered;
        Reordered.ScoreBoard = Board.ScoreBoard;
        for (int32 Index = Board.NumTiles() - 1; Index >= 0; Index--)
        {
            Reordered.SetTileData(Board.GetPositionAt(Index), Board.GetTileDataAt(Index));
        }
        TestTrue(TEXT("tile order doesn't matter"), Reordered.HasSameTilesAndScores(Board));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItTilePowerDataTest,
    "ConnectIt.Board.TilePowerData", TestFlags)

bool FConnectItTilePowerDataTest::RunTest(const FString& Parameters)
{
    // Stage 3 of tile powers: where powers are defined and how tiles carry
    // them. Nothing fires yet.
    const FGameplayTag PowerTag = TestTilePowerTag;
    const FGameplayTag OtherTag = TestOtherTilePowerTag;
    const FGameplayTag UndefinedTag = TestUndefinedTilePowerTag;
    if (!TestTrue(TEXT("the test power tags exist"), PowerTag.IsValid() && OtherTag.IsValid() && UndefinedTag.IsValid()))
    {
        return false;
    }
    const auto StrengthOf = [](const FConnectItTilePower* Power) -> int32
    {
        const FConnectItTilePowerEffect_Test* Effect = Power ? Power->Effect.GetPtr<FConnectItTilePowerEffect_Test>() : nullptr;
        return Effect ? Effect->Strength : INDEX_NONE;
    };
    const auto MakeEntry = [](FGameplayTag Tag, int32 Strength)
    {
        FConnectItTileCatalogEntry Entry;
        Entry.TileClass = AConnectIt_TileBase::StaticClass();
        Entry.TilePower.CooldownTurns = 2;
        FConnectItTilePowerEffect_Test Effect;
        Effect.TestTag = Tag;
        Effect.Strength = Strength;
        Entry.TilePower.Effect = TInstancedStruct<FConnectItTilePowerEffect>::Make<FConnectItTilePowerEffect_Test>(Effect);
        return Entry;
    };

    // A catalog defining two powers
    UConnectIt_TileCatalogDataAsset* Catalog = NewObject<UConnectIt_TileCatalogDataAsset>();
    Catalog->DefaultTileClass = AConnectIt_BuilderTile::StaticClass();
    Catalog->Tiles.Add(PowerTag, MakeEntry(PowerTag, 5));
    Catalog->Tiles.Add(OtherTag, MakeEntry(OtherTag, 9));

    // --- A fresh board and fresh rules have no powers ---
    {
        const FConnectItBoardState Board = MakeBoard(3);
        bool bAnyPower = false;
        for (int32 Index = 0; Index < Board.NumTiles(); Index++)
        {
            bAnyPower |= Board.GetTileDataAt(Index).TilePowerTag.IsValid();
        }
        TestFalse(TEXT("fresh board: no tile has a power"), bAnyPower);
        TestTrue(TEXT("fresh rules: no powers"), FConnectItRuleSet().TilePowers.IsEmpty() && FConnectItRuleSet().LevelWideTilePowers.IsEmpty());
    }

    // --- The catalog: one entry per tag, a class for each, powers named by their tag ---
    {
        TestTrue(TEXT("catalog: a tag's class"), Catalog->GetTileClassFor(PowerTag) == AConnectIt_TileBase::StaticClass());
        TestTrue(TEXT("catalog: no tag is the default class"), Catalog->GetTileClassFor(FGameplayTag()) == AConnectIt_BuilderTile::StaticClass());
        TestTrue(TEXT("catalog: an undefined tag has none"), Catalog->GetTileClassFor(UndefinedTag) == nullptr);

        const TArray<FConnectItTilePower> Powers = Catalog->MakeTilePowers();
        const FConnectItTilePower* Named = Powers.FindByPredicate([&PowerTag](const FConnectItTilePower& Power) { return Power.GetTag() == PowerTag; });
        TestTrue(TEXT("catalog: each power known by its effect's tag, with its values"),
            Powers.Num() == 2 && Named && StrengthOf(Named) == 5 && Named->CooldownTurns == 2);
    }

    // --- Rules: the catalog's powers and the level-wide tags ---
    {
        FConnectItRuleSet Rules;
        Rules.SetTilePowers(Catalog->MakeTilePowers(), { OtherTag, OtherTag, UndefinedTag });
        TestEqual(TEXT("rules: every catalog power"), Rules.TilePowers.Num(), 2);
        TestEqual(TEXT("rules: a tag finds its power and values"), StrengthOf(Rules.FindTilePower(PowerTag)), 5);
        TestNull(TEXT("rules: an undefined tag finds none"), Rules.FindTilePower(UndefinedTag));
        TestTrue(TEXT("rules: level-wide tags, a repeat and an undefined one skipped"),
            Rules.LevelWideTilePowers == TArray<FGameplayTag>({ OtherTag }));
    }

    // --- Building the board: a builder tile's tag reaches its tile data ---
    {
        AConnectIt_BuilderTile* Builder = GetMutableDefault<AConnectIt_BuilderTile>();
        const FConnectItTileInitialData Saved = Builder->TileData;
        ON_SCOPE_EXIT { Builder->TileData = Saved; };

        Builder->TileData.TilePowerTag = PowerTag;
        Builder->TileData.Multiplier = 2.f;
        const FConnectItTileData Tile = UConnectIt_BoardStateComponent::MakeTileData(Builder, 2, 1.f, Catalog);
        TestTrue(TEXT("build: the builder tile's tag and settings"), Tile.TilePowerTag == PowerTag && Tile.Multiplier == 2.f);

        Builder->TileData.TilePowerTag = UndefinedTag;
        const FConnectItTileData Undefined = UConnectIt_BoardStateComponent::MakeTileData(Builder, 2, 1.f, Catalog);
        TestFalse(TEXT("build: a tag the catalog doesn't define gives no power"), Undefined.TilePowerTag.IsValid());
    }

    // --- Tile ids: given by the board, never reused ---
    {
        FConnectItBoardState Board;
        Board.ScoreBoard = { 0.f, 0.f };
        const int32 First = Board.AddTile(FGridPosition(0, 0), FConnectItTileData());
        const int32 Second = Board.AddTile(FGridPosition(1, 0), FConnectItTileData());
        Board.RemoveTileData(FGridPosition(1, 0));
        const int32 Third = Board.AddTile(FGridPosition(1, 0), FConnectItTileData());
        TestTrue(TEXT("ids: each tile its own, a removed tile's id not reused"), First == 0 && Second == 1 && Third == 2);
    }

    // --- Add Tiles: a cell's tag and a new id; replayed the same ---
    {
        FConnectItRuleSet Rules;
        FConnectItBoardState Board = MakeBoard(3);
        Board.NextTileId = 9;

        FConnectItBoardOperation_AddTiles Add;
        Add.Faction = INDEX_NONE;
        FConnectItBoardLayoutCell& Powered = Add.Cells.AddDefaulted_GetRef();
        Powered.Position = FGridPosition(3, 0);
        Powered.TileData.TilePowerTag = PowerTag;
        Add.Cells.AddDefaulted_GetRef().Position = FGridPosition(3, 1);

        FConnectItBoardState After = Board;
        FConnectItBoardChangeEvent Events;
        Rules.CarryOutOperation(Add, After, &Events);
        const FConnectItTileData* Tile = After.GetTileData(FGridPosition(3, 0));
        TestTrue(TEXT("add: the cell's power tag"), Tile && Tile->TilePowerTag == PowerTag);
        TestTrue(TEXT("add: new ids from the board's counter"),
            Tile && Tile->TileId == 9 && After.GetTileData(FGridPosition(3, 1))->TileId == 10 && After.NextTileId == 11);
        TestFalse(TEXT("add: a cell with no tag gives none"), After.GetTileData(FGridPosition(3, 1))->TilePowerTag.IsValid());

        const auto* Added = EventAt<FConnectItBoardEvent_TilesAdded>(Events, 0);
        TestTrue(TEXT("add: the event carries the ids"), Added && Added->TileIds == TArray<int32>({ 9, 10 }));

        FConnectItBoardState Replayed = Board;
        FConnectItBoardEvent::ApplyAll(Events, Replayed);
        TestTrue(TEXT("add: before + events = after, ids and tags included"), Replayed.HasSameTilesAndScores(After));
        TestEqual(TEXT("add: the replayed board's counter moved on too"), Replayed.NextTileId, After.NextTileId);
    }

    // --- Shift: the tag, ready turn and id travel with the tile ---
    {
        FConnectItBoardState Board = MakeBoard(5);
        FConnectItTileData* Start = Board.GetTileDataMutable(FGridPosition(0, 0));
        Start->TilePowerTag = PowerTag;
        Start->TileId = 42;
        Start->TilePowerReadyTurn = 6;

        FConnectItBoardOperation_Shift Shift;
        Shift.Faction = 0;
        Shift.Direction = EGridDirection::Right;
        for (int32 X = 0; X < 5; X++)
        {
            Shift.Positions.Add(FGridPosition(X, 0));
        }
        FConnectItBoardChanges Changes;
        Shift.Apply(Board, Changes, nullptr);
        const FConnectItTileData* Moved = Board.GetTileData(FGridPosition(1, 0));
        TestTrue(TEXT("shift: the power moved with its tile"),
            Moved && Moved->TilePowerTag == PowerTag && Moved->TileId == 42 && Moved->TilePowerReadyTurn == 6);
        TestFalse(TEXT("shift: and left nothing behind"), Board.GetTileData(FGridPosition(0, 0))->TilePowerTag.IsValid());
    }

    // --- Validation: a catalog entry with no class or effect; level-wide tags named once ---
#if WITH_EDITOR
    {
        UConnectIt_TileCatalogDataAsset* Bad = NewObject<UConnectIt_TileCatalogDataAsset>();
        Bad->Tiles.Add(PowerTag, FConnectItTileCatalogEntry());
        Bad->Tiles.Add(OtherTag, MakeEntry(PowerTag, 1));
        FDataValidationContext CatalogContext;
        TestEqual(TEXT("catalog validation: fails"), Bad->IsDataValid(CatalogContext), EDataValidationResult::Invalid);
        TestTrue(TEXT("catalog validation: no class, no effect, an effect for another tag than its key"),
            CatalogContext.GetNumErrors() == 3);

        UConnectIt_LevelDirector* Director = NewObject<UConnectIt_LevelDirector>();
        Director->LevelWideTilePowers = { PowerTag, PowerTag, FGameplayTag() };
        FDataValidationContext Context;
        Director->IsDataValid(Context);
        const auto Reported = [&Context](const TCHAR* Text)
        {
            return Context.GetIssues().ContainsByPredicate([Text](const FDataValidationContext::FIssue& Issue)
            {
                return Issue.Message.ToString().Contains(Text);
            });
        };
        TestTrue(TEXT("director validation: a level-wide tag listed twice"), Reported(TEXT("twice")));
        TestTrue(TEXT("director validation: and an empty one"), Reported(TEXT("has no tag")));
    }
#endif

    // --- The turn: the AI's search counts one per ply ---
    {
        const FConnectItBoardState Board = MakeBoard(5);
        const FConnectItMinMaxRules Rules(Board, FConnectItRuleSet(), DefaultEvaluationWeights(), DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());
        FConnectItBoardState AtTurn = Board;
        AtTurn.TurnNumber = 7;
        const auto Root = Rules.MakeRoot(AtTurn, 0);
        const auto Child = Rules.ApplyMove(Root, PlaceMove(0, 0, 0));
        const auto Grandchild = Rules.ApplyMove(Child, PlaceMove(1, 1, 1));
        TestTrue(TEXT("turn: the root keeps the decision's turn, each ply adds one"),
            Root.Board.TurnNumber == 7 && Child.Board.TurnNumber == 8 && Grandchild.Board.TurnNumber == 9);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItTilePowerChainTest,
    "ConnectIt.Board.TilePowerChain", TestFlags)

bool FConnectItTilePowerChainTest::RunTest(const FString& Parameters)
{
    // Stage 4 of tile powers: firing them in the after-move chain, with
    // test-only powers (the first real powers are designed separately)
    using EPower = EConnectItTilePowerTrigger;
    if (!TestTrue(TEXT("the test power tags exist"), FGameplayTag(TestPower5).IsValid()))
    {
        return false;
    }
    const auto CarryOut = [](const FConnectItRuleSet& Rules, const FConnectItBoardOperation& Operation,
        FConnectItBoardState& Board, FConnectItBoardChangeEvent& OutEvents)
    {
        return Rules.CarryOutOperation(Operation, Board, &OutEvents);
    };
    const auto ReplaysTo = [](const FConnectItBoardState& Before, const FConnectItBoardChangeEvent& Events,
        const FConnectItBoardState& After)
    {
        FConnectItBoardState Replayed = Before;
        FConnectItBoardEvent::ApplyAll(Events, Replayed);
        return Replayed.HasSameTilesAndScores(After);
    };
    using FPlace = FConnectItBoardOperation_PlacePiece;

    // --- The owner's chain: place -> a line scores through a "move a
    //     neighbour" tile -> the moved piece completes a line -> that tile's
    //     Scoring power fires ---
    {
        FConnectItTilePowerEffect_TestMove Move;
        Move.From = FIntPoint(3, 5);   // (2,0) + (3,5) = (5,5)
        Move.To = FIntPoint(1, 3);     // (2,0) + (1,3) = (3,3)
        FConnectItRuleSet Rules;
        Rules.SetTilePowers({
            MakeTestPower(TestPower1, EPower::Scoring, Move),
            MakeTestPower(TestPower2, EPower::Scoring, FConnectItTilePowerEffect_Test()) }, {});

        FConnectItBoardState Board = MakeBoard(6);
        for (int32 X = 0; X < 3; X++)
        {
            Place(Board, X, 0, 0);
            Place(Board, X, 3, 0);
        }
        Place(Board, 5, 5, 0);
        SetPower(Board, 2, 0, TestPower1);
        SetPower(Board, 3, 3, TestPower2);

        FConnectItBoardState After = Board;
        FConnectItBoardChangeEvent Events;
        const float Points = CarryOut(Rules, MakeAt<FPlace>(0, 3, 0), After, Events);

        TestEqual(TEXT("chain: two lines scored"), Points, 8.f);
        TestTrue(TEXT("chain: the neighbour moved"), !After.IsTileOccupied(FGridPosition(5, 5)) && After.IsTileOccupied(FGridPosition(3, 3)));
        const bool bOrder = Events.Events.Num() == 7
            && EventAt<FConnectItBoardEvent_PiecePlaced>(Events, 0)
            && EventAt<FConnectItBoardEvent_Scored>(Events, 1)
            && EventAt<FConnectItBoardEvent_TilePowerTriggered>(Events, 2)
            && EventAt<FConnectItBoardEvent_PieceRemoved>(Events, 3)
            && EventAt<FConnectItBoardEvent_PiecePlaced>(Events, 4)
            && EventAt<FConnectItBoardEvent_Scored>(Events, 5)
            && EventAt<FConnectItBoardEvent_TilePowerTriggered>(Events, 6);
        if (TestTrue(TEXT("chain: placed, scored, mover fired, moved, scored, second power fired"), bOrder))
        {
            const auto* Mover = EventAt<FConnectItBoardEvent_TilePowerTriggered>(Events, 2);
            const auto* Second = EventAt<FConnectItBoardEvent_TilePowerTriggered>(Events, 6);
            TestTrue(TEXT("chain: the mover is the tile's power, for the scoring faction"),
                Mover->TilePowerName == TestPower1 && Mover->Position == FGridPosition(2, 0) && Mover->Faction == 0 && !Mover->bNoEffect);
            TestTrue(TEXT("chain: each event names its cause"),
                EventAt<FConnectItBoardEvent_PiecePlaced>(Events, 0)->CauseIndex == INDEX_NONE
                && EventAt<FConnectItBoardEvent_Scored>(Events, 1)->CauseIndex == INDEX_NONE
                && Mover->CauseIndex == 1
                && EventAt<FConnectItBoardEvent_PieceRemoved>(Events, 3)->CauseIndex == 2
                && EventAt<FConnectItBoardEvent_PiecePlaced>(Events, 4)->CauseIndex == 2
                && EventAt<FConnectItBoardEvent_Scored>(Events, 5)->CauseIndex == 4
                && Second->CauseIndex == 5);
            TestTrue(TEXT("chain: the second power had nothing to do"), Second->TilePowerName == TestPower2 && Second->bNoEffect);
        }
        TestTrue(TEXT("chain: before + events = after"), ReplaysTo(Board, Events, After));
    }

    // --- Settle until quiet; each power fires once per action ---
    {
        FConnectItRuleSet Rules;
        Rules.SetTilePowers({
            MakeTestPower(TestPower1, EPower::PieceEnter, PlaceAt({ FIntPoint(0, 2) })),
            MakeTestPower(TestPower2, EPower::PieceEnter, PlaceAt({ FIntPoint(0, -4) })) }, {});

        FConnectItBoardState Board = MakeBoard(6);
        SetPower(Board, 0, 0, TestPower1);   // -> (0,2)
        SetPower(Board, 0, 2, TestPower1);   // -> (0,4)
        SetPower(Board, 0, 4, TestPower2);   // -> back to (0,0), which has fired already

        FConnectItBoardState After = Board;
        FConnectItBoardChangeEvent Events;
        CarryOut(Rules, MakeAt<FPlace>(0, 0, 0), After, Events);
        const auto Triggered = PowersTriggered(Events);
        TestEqual(TEXT("settle: three powers, each once"), Triggered.Num(), 3);
        TestTrue(TEXT("settle: in the order they were set off"), Triggered.Num() == 3
            && Triggered[0]->Position == FGridPosition(0, 0) && Triggered[1]->Position == FGridPosition(0, 2)
            && Triggered[2]->Position == FGridPosition(0, 4));
        TestTrue(TEXT("settle: the last placement (back on the first tile) ends it"),
            Events.Events.Num() == 7 && EventAt<FConnectItBoardEvent_PiecePlaced>(Events, 6)
            && EventAt<FConnectItBoardEvent_PiecePlaced>(Events, 6)->Position == FGridPosition(0, 0));
        TestTrue(TEXT("settle: before + events = after"), ReplaysTo(Board, Events, After));
    }

    // --- Triggers: a capture is Piece Changed only; level-wide powers on every tile ---
    {
        FConnectItRuleSet Rules;
        const FConnectItTilePowerEffect_Test Nothing;
        Rules.SetTilePowers({
            MakeTestPower(TestPower1, EPower::PieceEnter, Nothing),
            MakeTestPower(TestPower2, EPower::PieceExit, Nothing),
            MakeTestPower(TestPower3, EPower::PieceChanged, Nothing) },
            { TestPower1, TestPower2, TestPower3 });

        FConnectItBoardState Board = MakeBoard(5);
        Place(Board, 1, 1, 1);
        const auto NamesAfter = [&](const FConnectItBoardOperation& Operation)
        {
            FConnectItBoardState After = Board;
            FConnectItBoardChangeEvent Events;
            Rules.CarryOutOperation(Operation, After, &Events);
            TArray<FGameplayTag> Names;
            for (const auto* Triggered : PowersTriggered(Events))
            {
                Names.Add(Triggered->TilePowerName);
            }
            return MakeTuple(Names, PowersTriggered(Events).Num() == 1 ? PowersTriggered(Events)[0]->Faction : INDEX_NONE);
        };
        const auto Captured = NamesAfter(MakeAt<FConnectItBoardOperation_CapturePiece>(0, 1, 1));
        TestTrue(TEXT("capture: Piece Changed only, for the new owner"),
            Captured.Key == TArray<FGameplayTag>({ TestPower3 }) && Captured.Value == 0);
        const auto Placed = NamesAfter(MakeAt<FPlace>(0, 2, 2));
        TestTrue(TEXT("place: Piece Enter only"), Placed.Key == TArray<FGameplayTag>({ TestPower1 }) && Placed.Value == 0);
        const auto Removed = NamesAfter(MakeAt<FConnectItBoardOperation_RemovePiece>(0, 1, 1));
        TestTrue(TEXT("remove: Piece Exit only, for the leaving piece's owner"),
            Removed.Key == TArray<FGameplayTag>({ TestPower2 }) && Removed.Value == 1);
    }

    // --- Cooldowns count turns; a power with nothing to do still shows and cools down ---
    for (int32 Cooldown = 0; Cooldown <= 2; Cooldown++)
    {
        FConnectItRuleSet Rules;
        Rules.SetTilePowers({ MakeTestPower(TestPower1, EPower::PieceEnter, FConnectItTilePowerEffect_Test(), Cooldown) }, {});

        FConnectItBoardState Board = MakeBoard(3);
        SetPower(Board, 0, 0, TestPower1);
        Board.TurnNumber = 5;

        FConnectItBoardState Fired = Board;
        FConnectItBoardChangeEvent Events;
        CarryOut(Rules, MakeAt<FPlace>(0, 0, 0), Fired, Events);
        const auto First = PowersTriggered(Events);
        TestTrue(FString::Printf(TEXT("cooldown %d: fires, with no effect, ready again at turn %d"), Cooldown, 5 + Cooldown),
            First.Num() == 1 && First[0]->bNoEffect && First[0]->ReadyTurn == 5 + Cooldown
            && TileAt(Fired, 0, 0).TilePowerReadyTurn == 5 + Cooldown);

        for (int32 Later = 0; Later <= 2; Later++)
        {
            FConnectItBoardState Again = Fired;
            Place(Again, 0, 0, -1);
            Again.TurnNumber = 5 + Later;
            FConnectItBoardChangeEvent AgainEvents;
            CarryOut(Rules, MakeAt<FPlace>(0, 0, 0), Again, AgainEvents);
            TestEqual(FString::Printf(TEXT("cooldown %d: %d turn(s) later it %s"), Cooldown, Later,
                Later >= Cooldown ? TEXT("fires") : TEXT("doesn't")),
                PowersTriggered(AgainEvents).Num(), Later >= Cooldown ? 1 : 0);
        }
    }

    // --- The gate: the tile's multiplier ---
    {
        FConnectItRuleSet Rules;
        Rules.SetTilePowers({ MakeTestPower(TestPower1, EPower::PieceEnter, FConnectItTilePowerEffect_Test(), 0, 2.f) }, {});
        FConnectItBoardState Board = MakeBoard(3);
        SetPower(Board, 0, 0, TestPower1);

        FConnectItBoardState Low = Board;
        FConnectItBoardChangeEvent LowEvents;
        CarryOut(Rules, MakeAt<FPlace>(0, 0, 0), Low, LowEvents);
        TestEqual(TEXT("gate: x1 is below x2, nothing fires"), PowersTriggered(LowEvents).Num(), 0);

        FConnectItBoardState High = Board;
        High.GetTileDataMutable(FGridPosition(0, 0))->Multiplier = 2.f;
        FConnectItBoardChangeEvent HighEvents;
        CarryOut(Rules, MakeAt<FPlace>(0, 0, 0), High, HighEvents);
        TestEqual(TEXT("gate: at x2 it fires"), PowersTriggered(HighEvents).Num(), 1);
    }

    // --- Random choices come from the board's seed ---
    {
        FConnectItRuleSet Rules;
        Rules.SetTilePowers({ MakeTestPower(TestPower1, EPower::PieceEnter, FConnectItTilePowerEffect_TestRandomPlace()) }, {});
        FConnectItBoardState Board = MakeBoard(5);
        SetPower(Board, 0, 0, TestPower1);
        Board.RandomSeed = 123;

        const auto RandomlyPlaced = [&](const FConnectItBoardState& From, FConnectItBoardState& OutAfter)
        {
            OutAfter = From;
            FConnectItBoardChangeEvent Events;
            CarryOut(Rules, MakeAt<FPlace>(0, 0, 0), OutAfter, Events);
            const auto* Placed = EventAt<FConnectItBoardEvent_PiecePlaced>(Events, 2);
            return Placed ? Placed->Position : FGridPosition(-1, -1);
        };
        FConnectItBoardState First;
        FConnectItBoardState Second;
        const FGridPosition FirstPick = RandomlyPlaced(Board, First);
        const FGridPosition SecondPick = RandomlyPlaced(Board, Second);
        TestTrue(TEXT("random: it placed somewhere"), FirstPick != FGridPosition(-1, -1));
        TestTrue(TEXT("random: the same seed, the same result"), FirstPick == SecondPick);
        TestTrue(TEXT("random: the seed moved on"), First.RandomSeed != Board.RandomSeed && First.RandomSeed == Second.RandomSeed);
    }

    // --- Sync: every settle pass, only when state calls for it, not once per action ---
    {
        FConnectItTilePowerEffect_TestSyncClear Clear;
        Clear.Watch = FIntPoint(1, 0);   // S (2,2) watches (3,2)
        FConnectItRuleSet Rules;
        Rules.SetTilePowers({
            MakeTestPower(TestPower1, EPower::PieceEnter, PlaceAt({ FIntPoint(2, 2), FIntPoint(0, 4) })),   // A (0,0) -> S, C
            MakeTestPower(TestPower2, EPower::PieceEnter, PlaceAt({ FIntPoint(2, -2) })),                   // C (0,4) -> S
            MakeTestPower(TestPower5, EPower::Sync, Clear) }, {});

        FConnectItBoardState Board = MakeBoard(6);
        SetPower(Board, 0, 0, TestPower1);
        SetPower(Board, 0, 4, TestPower2);
        SetPower(Board, 2, 2, TestPower5);

        const auto SyncsAfterPlacing = [&](FConnectItBoardState& InOutBoard, FConnectItBoardChangeEvent& OutEvents)
        {
            CarryOut(Rules, MakeAt<FPlace>(0, 0, 0), InOutBoard, OutEvents);
            int32 Syncs = 0;
            for (const auto* Triggered : PowersTriggered(OutEvents))
            {
                Syncs += Triggered->TilePowerName == TestPower5 ? 1 : 0;
            }
            return Syncs;
        };

        FConnectItBoardState Watched = Board;
        FConnectItBoardChangeEvent WatchedEvents;
        TestEqual(TEXT("sync: S was filled twice in one action and cleared twice"), SyncsAfterPlacing(Watched, WatchedEvents), 2);
        TestFalse(TEXT("sync: S ends empty"), Watched.IsTileOccupied(FGridPosition(2, 2)));
        TestTrue(TEXT("sync: before + events = after"), ReplaysTo(Board, WatchedEvents, Watched));

        FConnectItBoardState Kept = Board;
        Place(Kept, 3, 2, 1);
        FConnectItBoardChangeEvent KeptEvents;
        TestEqual(TEXT("sync: with the watched tile filled, nothing calls for it -- no event"), SyncsAfterPlacing(Kept, KeptEvents), 0);
        TestTrue(TEXT("sync: and S keeps its piece"), Kept.IsTileOccupied(FGridPosition(2, 2)));
    }

    // --- No powers: the chain is just scoring, as before ---
    {
        FConnectItBoardState Board = MakeBoard(5);
        FConnectItBoardChangeEvent Events;
        FConnectItRuleSet().CarryOutOperation(MakeAt<FPlace>(0, 0, 0), Board, &Events);
        TestEqual(TEXT("no powers: one event"), Events.Events.Num(), 1);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItTileLockTest,
    "ConnectIt.Board.TileLock", TestFlags)

bool FConnectItTileLockTest::RunTest(const FString& Parameters)
{
    // Tile lock plan, Stage 1: a locked tile is in play but can't be placed on,
    // and only operations that say so act on it
    FConnectItRuleSet Rules;
    const auto Lock = [](FConnectItBoardState& Board, int32 X, int32 Y, bool bLocked = true)
    {
        Board.GetTileDataMutable(FGridPosition(X, Y))->bIsLocked = bLocked;
    };
    const auto RefusedAsLocked = [&Rules](const FConnectItBoardOperation& Operation, const FConnectItBoardState& Board)
    {
        FString WhyNot;
        return !Rules.CanCarryOut(Operation, Board, &WhyNot) && WhyNot.Contains(TEXT("locked"));
    };

    // --- Placement ---
    {
        FConnectItBoardState Board = MakeBoard(3);
        Lock(Board, 1, 1);
        using FPlace = FConnectItBoardOperation_PlacePiece;
        TestFalse(TEXT("place: a locked tile isn't placeable"), FPlace::IsTilePlaceableAt(Board, FGridPosition(1, 1)));
        TestFalse(TEXT("place: refused"), Rules.CanCarryOut(MakeAt<FPlace>(0, 1, 1), Board));
        TestTrue(TEXT("place: the AI never tries it"), [&Board]()
        {
            bool bOffered = false;
            FPlace::ForEachMove(Board, 0, [&bOffered](const FPlace& Move) { bOffered |= Move.Position == FGridPosition(1, 1); });
            return !bOffered;
        }());
        TestEqual(TEXT("blocker: Locked"), UConnectIt_BoardStateLibrary::GetPlacementBlocker(Board, FGridPosition(1, 1)),
            EConnectItPlacementBlocker::Locked);
        TestEqual(TEXT("blocker: None on a free tile"), UConnectIt_BoardStateLibrary::GetPlacementBlocker(Board, FGridPosition(0, 0)),
            EConnectItPlacementBlocker::None);
        TestEqual(TEXT("blocker: Not A Tile"), UConnectIt_BoardStateLibrary::GetPlacementBlocker(Board, FGridPosition(9, 9)),
            EConnectItPlacementBlocker::NotATile);

        Lock(Board, 1, 1, false);
        TestTrue(TEXT("place: allowed once unlocked"), Rules.CanCarryOut(MakeAt<FPlace>(0, 1, 1), Board));
    }

    // --- Each operation on a locked target ---
    {
        FConnectItBoardState Board = MakeBoard(5);
        Place(Board, 1, 1, 1);
        Place(Board, 2, 2, 0);
        Board.GetTileDataMutable(FGridPosition(1, 1))->Multiplier = 3.f;
        Lock(Board, 1, 1);

        TestTrue(TEXT("capture: refused (locked)"), RefusedAsLocked(MakeAt<FConnectItBoardOperation_CapturePiece>(0, 1, 1), Board));
        TestTrue(TEXT("remove: refused (locked)"), RefusedAsLocked(MakeAt<FConnectItBoardOperation_RemovePiece>(0, 1, 1), Board));
        TestTrue(TEXT("destroy multiplier: refused (locked)"),
            RefusedAsLocked(MakeAt<FConnectItBoardOperation_DestroyTileMultiplier>(0, 1, 1), Board));
        TestTrue(TEXT("toggle: refused (locked)"), RefusedAsLocked(MakeAt<FConnectItBoardOperation_ToggleTileActive>(0, 1, 1), Board));
        FConnectItBoardOperation_SwapPieces Swap;
        Swap.Faction = 0;
        Swap.PositionA = FGridPosition(2, 2);
        Swap.PositionB = FGridPosition(1, 1);
        TestTrue(TEXT("swap: refused (one tile locked)"), RefusedAsLocked(Swap, Board));

        TestTrue(TEXT("force place: allowed"), Rules.CanCarryOut(MakeAt<FConnectItBoardOperation_ForcePlacePiece>(0, 1, 1), Board));
        FConnectItBoardOperation_RemoveTiles RemoveTiles;
        RemoveTiles.Positions = { FGridPosition(1, 1) };
        TestTrue(TEXT("remove tiles: allowed"), Rules.CanCarryOut(RemoveTiles, Board));
        FConnectItBoardOperation_Shift Shift;
        Shift.Faction = 0;
        Shift.Direction = EGridDirection::Right;
        for (int32 X = 0; X < 5; X++)
        {
            Shift.Positions.Add(FGridPosition(X, 1));
        }
        TestTrue(TEXT("shift: allowed"), Rules.CanCarryOut(Shift, Board));

        // The lock moves with its tile
        FConnectItBoardState Shifted = Board;
        FConnectItBoardChanges Changes;
        Shift.Apply(Shifted, Changes, nullptr);
        TestTrue(TEXT("shift: the lock moved with the tile"),
            TileAt(Shifted, 2, 1).bIsLocked && !TileAt(Shifted, 1, 1).bIsLocked && TileAt(Shifted, 2, 1).FactionPiece == 1);
    }

    // --- Set Tile Locked ---
    {
        FConnectItBoardState Board = MakeBoard(3);
        using FSetLocked = FConnectItBoardOperation_SetTileLocked;
        FSetLocked SetLocked = MakeAt<FSetLocked>(INDEX_NONE, 1, 1);
        TestTrue(TEXT("set locked: can apply"), Rules.CanCarryOut(SetLocked, Board));

        FConnectItBoardState After = Board;
        FConnectItBoardChangeEvent Events;
        const float Points = Rules.CarryOutOperation(SetLocked, After, &Events);
        TestTrue(TEXT("set locked: the tile is locked, nothing scored"), TileAt(After, 1, 1).bIsLocked && Points == 0.f);
        const auto* Changed = OnlyEvent<FConnectItBoardEvent_TileLockChanged>(Events);
        TestTrue(TEXT("set locked: one Tile Lock Changed"), Changed && Changed->Position == FGridPosition(1, 1) && Changed->bLocked);
        FConnectItBoardState Replayed = Board;
        FConnectItBoardEvent::ApplyAll(Events, Replayed);
        TestTrue(TEXT("set locked: before + events = after"), Replayed.HasSameTilesAndScores(After));

        TestFalse(TEXT("set locked: already locked is refused"), Rules.CanCarryOut(SetLocked, After));
        SetLocked.bLocked = false;
        TestTrue(TEXT("set locked: unlocking a locked tile is allowed"), Rules.CanCarryOut(SetLocked, After));
    }

    // --- Legality: only locked empty tiles left = no legal placement; locked pieces can't be swapped ---
    {
        FConnectItBoardState Board = MakeBoard(2);
        Place(Board, 0, 0, 0);
        Place(Board, 1, 0, 1);
        Lock(Board, 0, 1);
        Lock(Board, 1, 1);
        TestEqual(TEXT("legality: no placeable tile"),
            FConnectItBoardOperation::GetLegality(ConnectIt_BoardOperation_PlacePiece, Board, 0), EConnectItMoveLegality::NoLegalMove);
        TestEqual(TEXT("legality: a swap is still possible"),
            FConnectItBoardOperation::GetLegality(ConnectIt_BoardOperation_SwapPieces, Board, 0), EConnectItMoveLegality::Legal);
        Lock(Board, 1, 0);
        TestEqual(TEXT("legality: the only enemy piece is locked -- no swap"),
            FConnectItBoardOperation::GetLegality(ConnectIt_BoardOperation_SwapPieces, Board, 0), EConnectItMoveLegality::NoLegalMove);
    }

    // --- The AI: an empty locked tile blocks a line ---
    {
        FConnectItBoardState Open = MakeBoard(4);
        Place(Open, 0, 0, 0);
        Place(Open, 1, 0, 0);
        Place(Open, 2, 0, 0);
        FConnectItBoardState Blocked = Open;
        Lock(Blocked, 3, 0);
        const FConnectItMinMaxRules MinMax(Open, RulesWonAt(100.f), DefaultEvaluationWeights(), DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());
        TestTrue(TEXT("ai: three in a row is worth less when the fourth tile is locked"),
            MinMax.EvaluateState(MinMax.MakeRoot(Blocked, 0)) < MinMax.EvaluateState(MinMax.MakeRoot(Open, 0)));
    }

    // --- Tile powers respect the lock ---
    {
        FConnectItTilePowerEffect_TestMove Move;
        Move.From = FIntPoint(1, 1);   // (0,0) + (1,1) = the locked (1,1)
        Move.To = FIntPoint(2, 0);
        Rules.SetTilePowers({ MakeTestPower(TestPower1, EConnectItTilePowerTrigger::PieceEnter, Move) }, {});
        FConnectItBoardState Board = MakeBoard(3);
        Board.GetTileDataMutable(FGridPosition(0, 0))->TilePowerTag = TestPower1;
        Place(Board, 1, 1, 1);
        Lock(Board, 1, 1);

        FConnectItBoardChangeEvent Events;
        Rules.CarryOutOperation(MakeAt<FConnectItBoardOperation_PlacePiece>(0, 0, 0), Board, &Events);
        const auto Triggered = PowersTriggered(Events);
        TestTrue(TEXT("powers: the power fired but couldn't take the locked tile's piece"),
            Triggered.Num() == 1 && Triggered[0]->bNoEffect == false && TileAt(Board, 1, 1).FactionPiece == 1);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItChainedTilesTest,
    "ConnectIt.Board.ChainedTiles", TestFlags)

bool FConnectItChainedTilesTest::RunTest(const FString& Parameters)
{
    // Tile lock plan, Stage 2: a Chained tile (a Sync power) is locked exactly
    // while its partner has no piece; the partner is a per-tile link (tile id)
    FConnectItRuleSet Rules;
    Rules.SetTilePowers({ MakeTestPower(ConnectIt_TilePower_Chained, EConnectItTilePowerTrigger::Sync, FConnectItTilePowerEffect_Chained()) }, {});

    // A 5x5 board with tile ids; Chained tiles at Chains, all naming the partner at (0,0)
    const auto MakeChainBoard = [](std::initializer_list<FGridPosition> Chains)
    {
        FConnectItBoardState Board = MakeBoard(5);
        for (int32 Index = 0; Index < Board.NumTiles(); Index++)
        {
            Board.TileDataArray[Index].TileId = Index;
        }
        Board.NextTileId = Board.NumTiles();
        for (const FGridPosition& Chain : Chains)
        {
            Board.GetTileDataMutable(Chain)->TilePowerTag = ConnectIt_TilePower_Chained;
            Board.LinkChainPartner(Chain, true, FGridPosition(0, 0));
        }
        return Board;
    };
    const auto Settle = [&Rules](FConnectItBoardState& Board)
    {
        FConnectItBoardChangeEvent Events;
        Rules.ResolveBoardChange(Board, FConnectItBoardChanges(), &Events);
        return Events;
    };
    const auto CarryOut = [&Rules](const FConnectItBoardOperation& Operation, FConnectItBoardState& Board)
    {
        FConnectItBoardChangeEvent Events;
        if (Rules.CanCarryOut(Operation, Board)) Rules.CarryOutOperation(Operation, Board, &Events);
        return Events;
    };
    const auto ReplaysTo = [](const FConnectItBoardState& Before, const FConnectItBoardChangeEvent& Events, const FConnectItBoardState& After)
    {
        FConnectItBoardState Replayed = Before;
        FConnectItBoardEvent::ApplyAll(Events, Replayed);
        return Replayed.HasSameTilesAndScores(After);
    };
    const FGridPosition C(4, 4);
    using FPlace = FConnectItBoardOperation_PlacePiece;

    // --- The seeded board settles: locked while the partner is empty ---
    {
        const FConnectItBoardState Built = MakeChainBoard({ C });
        FConnectItBoardState Seeded = Built;
        const FConnectItBoardChangeEvent Events = Settle(Seeded);
        TestTrue(TEXT("seed: the chained tile starts locked"), TileAt(Seeded, 4, 4).bIsLocked);
        TestTrue(TEXT("seed: Power Triggered, then Tile Lock Changed"), Events.Events.Num() == 2
            && EventAt<FConnectItBoardEvent_TilePowerTriggered>(Events, 0) && EventAt<FConnectItBoardEvent_TileLockChanged>(Events, 1));
        TestTrue(TEXT("seed: before + events = after"), ReplaysTo(Built, Events, Seeded));

        FConnectItBoardState WithPiece = MakeChainBoard({ C });
        Place(WithPiece, 0, 0, 0);
        TestEqual(TEXT("seed: a partner with a piece -- unlocked, nothing to say"), Settle(WithPiece).Events.Num(), 0);
        TestFalse(TEXT("seed: and it stays unlocked"), TileAt(WithPiece, 4, 4).bIsLocked);
    }

    // --- Place on the partner unlocks; emptying it destroys the piece, then locks ---
    {
        FConnectItBoardState Board = MakeChainBoard({ C });
        Settle(Board);
        TestFalse(TEXT("chain: can't place on the locked chained tile"), Rules.CanCarryOut(MakeAt<FPlace>(1, 4, 4), Board));

        CarryOut(MakeAt<FPlace>(0, 0, 0), Board);
        TestFalse(TEXT("chain: a piece on the partner unlocks it"), TileAt(Board, 4, 4).bIsLocked);
        CarryOut(MakeAt<FPlace>(1, 4, 4), Board);
        TestEqual(TEXT("chain: now it takes a piece"), TileAt(Board, 4, 4).FactionPiece, 1);

        const FConnectItBoardState Before = Board;
        const FConnectItBoardChangeEvent Events = CarryOut(MakeAt<FConnectItBoardOperation_RemovePiece>(1, 0, 0), Board);
        TestTrue(TEXT("chain: the partner emptied -- the piece goes, then the lock"),
            !Board.IsTileOccupied(C) && TileAt(Board, 4, 4).bIsLocked);
        TestTrue(TEXT("chain: removed, power, removed, locked"), Events.Events.Num() == 4
            && EventAt<FConnectItBoardEvent_PieceRemoved>(Events, 0) && EventAt<FConnectItBoardEvent_TilePowerTriggered>(Events, 1)
            && EventAt<FConnectItBoardEvent_PieceRemoved>(Events, 2) && EventAt<FConnectItBoardEvent_TileLockChanged>(Events, 3));
        TestTrue(TEXT("chain: before + events = after"), ReplaysTo(Before, Events, Board));
    }

    // --- Scoring clears the partner: the chain closes after scoring ---
    {
        FConnectItBoardState Board = MakeChainBoard({ C });
        Place(Board, 0, 0, 0);
        Place(Board, 1, 0, 0);
        Place(Board, 2, 0, 0);
        Place(Board, 4, 4, 1);
        Settle(Board);
        TestFalse(TEXT("scoring: unlocked to start"), TileAt(Board, 4, 4).bIsLocked);

        CarryOut(MakeAt<FPlace>(0, 3, 0), Board);
        TestTrue(TEXT("scoring: the partner was cleared by the line"), !Board.IsTileOccupied(FGridPosition(0, 0)));
        TestTrue(TEXT("scoring: so the chained tile lost its piece and locked"),
            !Board.IsTileOccupied(C) && TileAt(Board, 4, 4).bIsLocked);
    }

    // --- A capture on the partner changes nothing ---
    {
        FConnectItBoardState Board = MakeChainBoard({ C });
        Place(Board, 0, 0, 1);
        Settle(Board);
        const FConnectItBoardChangeEvent Events = CarryOut(MakeAt<FConnectItBoardOperation_CapturePiece>(0, 0, 0), Board);
        TestTrue(TEXT("capture: still unlocked, no power fired"),
            !TileAt(Board, 4, 4).bIsLocked && PowersTriggered(Events).IsEmpty());
    }

    // --- No partner: stays locked ---
    {
        FConnectItBoardState Board = MakeChainBoard({ C });
        Board.LinkChainPartner(C, false, FGridPosition());
        Settle(Board);
        CarryOut(MakeAt<FPlace>(0, 0, 0), Board);
        TestTrue(TEXT("no partner: locked, whatever is placed"), TileAt(Board, 4, 4).bIsLocked);
        FGridPosition Partner;
        TestFalse(TEXT("no partner: Get Chain Partner says none"), UConnectIt_BoardStateLibrary::GetChainPartner(Board, C, Partner));
    }

    // --- Two chained tiles on one partner open and close together ---
    {
        FConnectItBoardState Board = MakeChainBoard({ C, FGridPosition(4, 2) });
        Settle(Board);
        const TArray<FGridPosition> Chained = UConnectIt_BoardStateLibrary::GetChainedTilesFor(Board, FGridPosition(0, 0));
        TestTrue(TEXT("two: the partner knows both"), Chained.Num() == 2 && Chained.Contains(C) && Chained.Contains(FGridPosition(4, 2)));

        CarryOut(MakeAt<FPlace>(0, 0, 0), Board);
        TestTrue(TEXT("two: both open"), !TileAt(Board, 4, 4).bIsLocked && !TileAt(Board, 4, 2).bIsLocked);
        CarryOut(MakeAt<FConnectItBoardOperation_RemovePiece>(1, 0, 0), Board);
        TestTrue(TEXT("two: both close"), TileAt(Board, 4, 4).bIsLocked && TileAt(Board, 4, 2).bIsLocked);
    }

    // --- Set Chain Partner, mid-match ---
    {
        FConnectItBoardState Board = MakeChainBoard({ C });
        Board.LinkChainPartner(C, false, FGridPosition());
        Place(Board, 2, 2, 0);
        Settle(Board);

        FConnectItBoardOperation_SetChainPartner Link;
        Link.Position = C;
        Link.PartnerPosition = FGridPosition(2, 2);
        TestTrue(TEXT("set partner: allowed on the locked chained tile"), Rules.CanCarryOut(Link, Board));
        const FConnectItBoardState Before = Board;
        const FConnectItBoardChangeEvent Events = CarryOut(Link, Board);
        TestFalse(TEXT("set partner: linked to a tile with a piece -- open in the same change"), TileAt(Board, 4, 4).bIsLocked);
        const auto* Changed = EventAt<FConnectItBoardEvent_ChainPartnerChanged>(Events, 0);
        TestTrue(TEXT("set partner: Chain Partner Changed first"), Changed && Changed->bHasPartner && Changed->PartnerPosition == FGridPosition(2, 2));
        TestTrue(TEXT("set partner: before + events = after"), ReplaysTo(Before, Events, Board));

        Link.bHasPartner = false;
        CarryOut(Link, Board);
        TestTrue(TEXT("set partner: unlinked -- locked again"), TileAt(Board, 4, 4).bIsLocked);

        FConnectItBoardOperation_SetChainPartner NotChained;
        NotChained.Position = FGridPosition(1, 1);
        NotChained.PartnerPosition = FGridPosition(2, 2);
        TestFalse(TEXT("set partner: refused on a tile that isn't Chained"), Rules.CanCarryOut(NotChained, Board));
    }

    // --- A shift keeps the link; removing the partner's tile unlinks ---
    {
        FConnectItBoardState Board = MakeChainBoard({ C });
        Place(Board, 0, 0, 0);
        Settle(Board);
        FConnectItBoardOperation_Shift Shift;
        Shift.Faction = 0;
        Shift.Direction = EGridDirection::Right;
        for (int32 X = 0; X < 5; X++)
        {
            Shift.Positions.Add(FGridPosition(X, 0));
        }
        CarryOut(Shift, Board);
        FGridPosition Partner;
        TestTrue(TEXT("shift: the partner moved and is still the partner"),
            UConnectIt_BoardStateLibrary::GetChainPartner(Board, C, Partner) && Partner == FGridPosition(1, 0));
        TestFalse(TEXT("shift: still open"), TileAt(Board, 4, 4).bIsLocked);

        FConnectItBoardOperation_RemoveTiles Remove;
        Remove.Positions = { FGridPosition(1, 0) };
        CarryOut(Remove, Board);
        TestTrue(TEXT("remove tiles: the partner is gone -- unpartnered and locked"),
            !UConnectIt_BoardStateLibrary::GetChainPartner(Board, C, Partner) && TileAt(Board, 4, 4).bIsLocked);
    }

    // --- Add Tiles links a cell's partner (one added alongside), replayed the same ---
    {
        FConnectItBoardState Board = MakeChainBoard({});
        FConnectItBoardOperation_AddTiles Add;
        Add.Faction = INDEX_NONE;
        FConnectItBoardLayoutCell& Chain = Add.Cells.AddDefaulted_GetRef();
        Chain.Position = FGridPosition(5, 0);
        Chain.TileData.TilePowerTag = ConnectIt_TilePower_Chained;
        Chain.TileData.bHasChainPartner = true;
        Chain.TileData.ChainPartner = FGridPosition(5, 1);
        FConnectItBoardLayoutCell& Partner = Add.Cells.AddDefaulted_GetRef();
        Partner.Position = FGridPosition(5, 1);

        const FConnectItBoardState Before = Board;
        const FConnectItBoardChangeEvent Events = CarryOut(Add, Board);
        TestEqual(TEXT("add tiles: the partner's id"), TileAt(Board, 5, 0).ChainPartnerId, TileAt(Board, 5, 1).TileId);
        TestTrue(TEXT("add tiles: the empty partner locks it in the same change"), TileAt(Board, 5, 0).bIsLocked);
        TestTrue(TEXT("add tiles: before + events = after"), ReplaysTo(Before, Events, Board));
    }

    // --- Simulate Operation and the AI see the same ---
    {
        FConnectItBoardState Board = MakeChainBoard({ C });
        Settle(Board);
        FConnectItBoardState Simulated;
        float Points = 0.f;
        int32 NumScored = 0;
        FConnectItBoardChangeEvent Events;
        UConnectIt_BoardStateLibrary::SimulateOperation(Board, Rules, FInstancedStruct::Make(MakeAt<FPlace>(0, 0, 0)), 0, -1,
            Simulated, Points, NumScored, Events);
        TestFalse(TEXT("simulate: the chain opens"), TileAt(Simulated, 4, 4).bIsLocked);

        const FConnectItMinMaxRules MinMax(Board, Rules, DefaultEvaluationWeights(), DefaultOrderingWeights(), PlaceOnly(), PlaceOnly());
        const auto Child = MinMax.ApplyMove(MinMax.MakeRoot(Board, 0), PlaceMove(0, 0, 0));
        TestFalse(TEXT("ai: its search opens the chain too"), TileAt(Child.Board, 4, 4).bIsLocked);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItMoveAndMultiplierTest,
    "ConnectIt.Board.MoveAndMultiplier", TestFlags)

bool FConnectItMoveAndMultiplierTest::RunTest(const FString& Parameters)
{
    // First tile powers plan, Stage 1: Move Piece, Adjust Multiplier, and each
    // tile type's multiplier floor and ceiling
    const auto CarryOut = [](const FConnectItRuleSet& Rules, const FConnectItBoardOperation& Operation, FConnectItBoardState& Board)
    {
        FConnectItBoardChangeEvent Events;
        if (Rules.CanCarryOut(Operation, Board)) Rules.CarryOutOperation(Operation, Board, &Events);
        return Events;
    };
    const auto ReplaysTo = [](const FConnectItBoardState& Before, const FConnectItBoardChangeEvent& Events, const FConnectItBoardState& After)
    {
        FConnectItBoardState Replayed = Before;
        FConnectItBoardEvent::ApplyAll(Events, Replayed);
        return Replayed.HasSameTilesAndScores(After);
    };
    const auto Move = [](int32 FromX, int32 FromY, int32 ToX, int32 ToY)
    {
        FConnectItBoardOperation_MovePiece Operation;
        Operation.Faction = 0;
        Operation.From = FGridPosition(FromX, FromY);
        Operation.To = FGridPosition(ToX, ToY);
        return Operation;
    };
    const auto Adjust = [](int32 X, int32 Y, float Delta)
    {
        FConnectItBoardOperation_AdjustMultiplier Operation;
        Operation.Position = FGridPosition(X, Y);
        Operation.Delta = Delta;
        return Operation;
    };
    // A tile type with limits: TestPower1, floor 0, ceiling 2; TestPower2, floor 2
    FConnectItRuleSet Rules;
    {
        FConnectItTilePower Capped = MakeTestPower(TestPower1, EConnectItTilePowerTrigger::PieceEnter, FConnectItTilePowerEffect_Test());
        Capped.MultiplierFloor = 0.f;
        Capped.MultiplierCeiling = 2.f;
        FConnectItTilePower Floored = MakeTestPower(TestPower2, EConnectItTilePowerTrigger::PieceEnter, FConnectItTilePowerEffect_Test());
        Floored.MultiplierFloor = 2.f;
        FConnectItTilePower Silent = MakeTestPower(TestPower3, EConnectItTilePowerTrigger::PieceEnter, FConnectItTilePowerEffect_Test());
        Silent.MultiplierFloor = 0.f;
        Silent.MultiplierCeiling = 0.f;
        Rules.SetTilePowers({ Capped, Floored, Silent }, {});
    }

    // --- Move Piece ---
    {
        FConnectItBoardState Board = MakeBoard(4);
        Place(Board, 1, 1, 1);
        const FConnectItBoardState Before = Board;
        const FConnectItBoardChangeEvent Events = CarryOut(Rules, Move(1, 1, 2, 1), Board);
        TestTrue(TEXT("move: the piece moved, keeping its owner (not the mover's)"),
            !Board.IsTileOccupied(FGridPosition(1, 1)) && TileAt(Board, 2, 1).FactionPiece == 1);
        const auto* Moved = OnlyEvent<FConnectItBoardEvent_PieceMoved>(Events);
        TestTrue(TEXT("move: one Piece Moved"), Moved && Moved->From == FGridPosition(1, 1) && Moved->To == FGridPosition(2, 1) && Moved->Faction == 1);
        TestTrue(TEXT("move: before + events = after"), ReplaysTo(Before, Events, Board));

        FConnectItBoardChanges Changes;
        FConnectItBoardState Again = Before;
        Move(1, 1, 2, 1).Apply(Again, Changes, nullptr);
        TestTrue(TEXT("move: an exit from, an enter to"),
            Changes.Exited.Num() == 1 && Changes.Exited[0].Position == FGridPosition(1, 1) && Changes.Exited[0].Faction == 1
            && Changes.Entered.Num() == 1 && Changes.Entered[0].Position == FGridPosition(2, 1));

        FConnectItBoardState Blocked = Before;
        Place(Blocked, 2, 1, 0);
        TestFalse(TEXT("move: not onto a piece"), Rules.CanCarryOut(Move(1, 1, 2, 1), Blocked));
        Blocked = Before;
        Blocked.GetTileDataMutable(FGridPosition(2, 1))->bIsActive = false;
        TestFalse(TEXT("move: not onto an inactive tile"), Rules.CanCarryOut(Move(1, 1, 2, 1), Blocked));
        Blocked = Before;
        Blocked.GetTileDataMutable(FGridPosition(2, 1))->bIsLocked = true;
        TestFalse(TEXT("move: not onto a locked tile"), Rules.CanCarryOut(Move(1, 1, 2, 1), Blocked));
        Blocked = Before;
        Blocked.GetTileDataMutable(FGridPosition(1, 1))->bIsLocked = true;
        TestFalse(TEXT("move: not off a locked tile"), Rules.CanCarryOut(Move(1, 1, 2, 1), Blocked));
        TestFalse(TEXT("move: nothing to move"), Rules.CanCarryOut(Move(0, 0, 2, 1), Before));
    }

    // --- Adjust Multiplier, within limits ---
    {
        FConnectItBoardState Board = MakeBoard(3);
        const FConnectItBoardState Before = Board;
        const FConnectItBoardChangeEvent Events = CarryOut(Rules, Adjust(0, 0, 2.f), Board);
        TestEqual(TEXT("adjust: a plain tile +2"), TileAt(Board, 0, 0).Multiplier, 3.f);
        const auto* Changed = OnlyEvent<FConnectItBoardEvent_MultiplierChanged>(Events);
        TestTrue(TEXT("adjust: Multiplier Changed 1 -> 3"), Changed && Changed->PreviousMultiplier == 1.f && Changed->NewMultiplier == 3.f);
        TestTrue(TEXT("adjust: before + events = after"), ReplaysTo(Before, Events, Board));

        CarryOut(Rules, Adjust(0, 0, -5.f), Board);
        TestEqual(TEXT("adjust: a plain tile's floor is 1"), TileAt(Board, 0, 0).Multiplier, 1.f);

        Board.GetTileDataMutable(FGridPosition(1, 1))->TilePowerTag = TestPower1;
        CarryOut(Rules, Adjust(1, 1, 5.f), Board);
        TestEqual(TEXT("adjust: a capped tile stops at its ceiling (2)"), TileAt(Board, 1, 1).Multiplier, 2.f);
        CarryOut(Rules, Adjust(1, 1, -5.f), Board);
        TestEqual(TEXT("adjust: and goes down to its floor (0)"), TileAt(Board, 1, 1).Multiplier, 0.f);

        TestFalse(TEXT("adjust: a change of 0 is refused"), Rules.CanCarryOut(Adjust(0, 0, 0.f), Board));
        Board.GetTileDataMutable(FGridPosition(2, 2))->bIsLocked = true;
        TestFalse(TEXT("adjust: not on a locked tile"), Rules.CanCarryOut(Adjust(2, 2, 1.f), Board));
    }

    // --- Limits through scoring: a ceiling-0 tile forms the line but adds nothing ---
    {
        FConnectItBoardState Board = MakeBoard(4);
        Place(Board, 0, 0, 0);
        Place(Board, 1, 0, 0);
        Place(Board, 2, 0, 0);
        FConnectItTileData* Silent = Board.GetTileDataMutable(FGridPosition(1, 0));
        Silent->TilePowerTag = TestPower3;
        Silent->Multiplier = 0.f;
        FConnectItTileData* Capped = Board.GetTileDataMutable(FGridPosition(2, 0));
        Capped->TilePowerTag = TestPower1;
        Capped->Multiplier = 2.f;

        const FConnectItBoardState Before = Board;
        FConnectItBoardChangeEvent Events;
        const float Points = Rules.CarryOutOperation(MakeAt<FConnectItBoardOperation_PlacePiece>(0, 3, 0), Board, &Events);
        TestEqual(TEXT("scoring: 1 + 0 + 2 + 1"), Points, 4.f);
        TestTrue(TEXT("scoring: the silent tile stays at 0, the capped one at 2, a plain one rises"),
            TileAt(Board, 1, 0).Multiplier == 0.f && TileAt(Board, 2, 0).Multiplier == 2.f && TileAt(Board, 0, 0).Multiplier == 2.f);
        TestTrue(TEXT("scoring: before + events = after (Scored carries the clamped values)"), ReplaysTo(Before, Events, Board));
    }

    // --- Destroy Multiplier resets within limits ---
    {
        FConnectItBoardState Board = MakeBoard(3);
        FConnectItTileData* Floored = Board.GetTileDataMutable(FGridPosition(1, 1));
        Floored->TilePowerTag = TestPower2;
        Floored->Multiplier = 3.f;
        const FConnectItBoardState Before = Board;
        const FConnectItBoardChangeEvent Events = CarryOut(Rules, MakeAt<FConnectItBoardOperation_DestroyTileMultiplier>(0, 1, 1), Board);
        TestEqual(TEXT("destroy: back to its floor (2), not 1"), TileAt(Board, 1, 1).Multiplier, 2.f);
        const auto* Destroyed = OnlyEvent<FConnectItBoardEvent_TileMultiplierDestroyed>(Events);
        TestTrue(TEXT("destroy: the event says 3 -> 2"), Destroyed && Destroyed->PreviousMultiplier == 3.f && Destroyed->NewMultiplier == 2.f);
        TestTrue(TEXT("destroy: before + events = after"), ReplaysTo(Before, Events, Board));
    }

    // --- Tiles joining the board start within limits ---
    {
        FConnectItBoardState Board = MakeBoard(3);
        FConnectItBoardOperation_AddTiles Add;
        Add.Faction = INDEX_NONE;
        FConnectItBoardLayoutCell& Cell = Add.Cells.AddDefaulted_GetRef();
        Cell.Position = FGridPosition(3, 0);
        Cell.TileData.TilePowerTag = TestPower1;
        Cell.TileData.Multiplier = 5.f;
        const FConnectItBoardState Before = Board;
        const FConnectItBoardChangeEvent Events = CarryOut(Rules, Add, Board);
        TestEqual(TEXT("add: a capped tile starts at its ceiling"), TileAt(Board, 3, 0).Multiplier, 2.f);
        TestTrue(TEXT("add: before + events = after"), ReplaysTo(Before, Events, Board));

        UConnectIt_TileCatalogDataAsset* Catalog = NewObject<UConnectIt_TileCatalogDataAsset>();
        FConnectItTileCatalogEntry Entry;
        Entry.TilePower.MultiplierCeiling = 2.f;
        Catalog->Tiles.Add(TestPower1, Entry);
        AConnectIt_BuilderTile* Builder = GetMutableDefault<AConnectIt_BuilderTile>();
        const FConnectItTileInitialData Saved = Builder->TileData;
        ON_SCOPE_EXIT { Builder->TileData = Saved; };
        Builder->TileData.TilePowerTag = TestPower1;
        Builder->TileData.Multiplier = 5.f;
        TestEqual(TEXT("build: a capped builder tile starts at its ceiling"),
            UConnectIt_BoardStateComponent::MakeTileData(Builder, 2, 1.f, Catalog).Multiplier, 2.f);
        Builder->TileData.TilePowerTag = FGameplayTag();
        Builder->TileData.Multiplier = 0.5f;
        TestEqual(TEXT("build: a plain tile starts at least at 1"),
            UConnectIt_BoardStateComponent::MakeTileData(Builder, 2, 1.f, Catalog).Multiplier, 1.f);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItPushAndVacuumTest,
    "ConnectIt.Board.PushAndVacuum", TestFlags)

bool FConnectItPushAndVacuumTest::RunTest(const FString& Parameters)
{
    // First tile powers plan, Stage 2: trigger sets, Push and Vacuum
    using EPower = EConnectItTilePowerTrigger;
    FConnectItTilePower Push = MakeTestPower(ConnectIt_TilePower_Push, EPower::Scoring, FConnectItTilePowerEffect_Push());
    FConnectItTilePower Vacuum = MakeTestPower(ConnectIt_TilePower_Vacuum, EPower::PieceExit, FConnectItTilePowerEffect_Vacuum());
    Vacuum.AddTrigger(EPower::Scoring);
    FConnectItRuleSet Rules;
    Rules.SetTilePowers({ Push, Vacuum }, {});

    // --- Each shipped effect names its own tag; the gate is the effect's ---
    {
        TestEqual(TEXT("tags: Push"), FConnectItTilePowerEffect_Push().GetPowerTag(), FGameplayTag(ConnectIt_TilePower_Push));
        TestEqual(TEXT("tags: Vacuum"), FConnectItTilePowerEffect_Vacuum().GetPowerTag(), FGameplayTag(ConnectIt_TilePower_Vacuum));
        TestEqual(TEXT("tags: Repel"), FConnectItTilePowerEffect_Repel().GetPowerTag(), FGameplayTag(ConnectIt_TilePower_Repel));
        TestEqual(TEXT("tags: Chained"), FConnectItTilePowerEffect_Chained().GetPowerTag(), FGameplayTag(ConnectIt_TilePower_Chained));
        TestEqual(TEXT("tags: a power is known by its effect's"), Push.GetTag(), FGameplayTag(ConnectIt_TilePower_Push));
        TestFalse(TEXT("tags: no effect, no power"), FConnectItTilePower().IsSet());

        FConnectItBoardState GateBoard = MakeBoard(3);
        FRandomStream Stream(0);
        const FConnectItTilePowerContext AtTile{ GateBoard, FGridPosition(0, 0), 0, Stream, nullptr };
        FConnectItTilePowerEffect_Push Gated;
        TestTrue(TEXT("gate: none by default"), Gated.PassesGate(AtTile));
        Gated.Gate.FireAtMultiplier = 2.f;
        TestFalse(TEXT("gate: Push at x2, the tile at x1: closed"), Gated.PassesGate(AtTile));
        GateBoard.GetTileDataMutable(FGridPosition(0, 0))->Multiplier = 2.f;
        TestTrue(TEXT("gate: the tile at x2: open"), Gated.PassesGate(AtTile));
    }

    // The operations an effect wants on Board at Position (seed Seed)
    const auto MovesFrom = [&Rules](const FConnectItTilePowerEffect& Effect, const FConnectItBoardState& Board, FGridPosition Position, int32 Seed)
    {
        FRandomStream Random(Seed);
        TArray<FInstancedStruct> Operations;
        Effect.MakeOperations(FConnectItTilePowerContext{ Board, Position, 0, Random, &Rules }, Operations);
        TArray<FConnectItBoardOperation_MovePiece> Moves;
        for (const FInstancedStruct& Entry : Operations)
        {
            if (const auto* Move = Entry.GetPtr<FConnectItBoardOperation_MovePiece>()) Moves.Add(*Move);
        }
        return Moves;
    };
    const auto ReplaysTo = [](const FConnectItBoardState& Before, const FConnectItBoardChangeEvent& Events, const FConnectItBoardState& After)
    {
        FConnectItBoardState Replayed = Before;
        FConnectItBoardEvent::ApplyAll(Events, Replayed);
        return Replayed.HasSameTilesAndScores(After);
    };

    // --- Trigger sets ---
    TestTrue(TEXT("triggers: Vacuum has Exit and Scoring, not Enter"),
        Vacuum.HasTrigger(EPower::PieceExit) && Vacuum.HasTrigger(EPower::Scoring) && !Vacuum.HasTrigger(EPower::PieceEnter));
    {
        UConnectIt_TileCatalogDataAsset* Catalog = NewObject<UConnectIt_TileCatalogDataAsset>();
        FConnectItTileCatalogEntry Entry;
        Entry.TileClass = AConnectIt_TileBase::StaticClass();
        Entry.TilePower = MakeTestPower(TestPower1, EPower::Sync, FConnectItTilePowerEffect_Test());
        Entry.TilePower.AddTrigger(EPower::PieceEnter);
        Catalog->Tiles.Add(TestPower1, Entry);
#if WITH_EDITOR
        FDataValidationContext Context;
        TestEqual(TEXT("triggers: Sync with another trigger fails validation"), Catalog->IsDataValid(Context), EDataValidationResult::Invalid);
#endif
    }

    // --- Push: only valid pushes; a single one is always the one ---
    {
        const FConnectItTilePowerEffect_Push Effect;
        FConnectItBoardState Board = MakeBoard(5);
        Place(Board, 2, 2, 0);   // its own piece: never pushed
        Place(Board, 2, 3, 1);   // (2,4) behind it is free: valid
        Place(Board, 1, 2, 0);   // (0,2) behind it is taken: not
        Place(Board, 0, 2, 1);
        Place(Board, 3, 2, 1);   // (4,2) behind it is locked: not
        Board.GetTileDataMutable(FGridPosition(4, 2))->bIsLocked = true;
        bool bAlwaysTheOne = true;
        for (int32 Seed = 1; Seed <= 8; Seed++)
        {
            const TArray<FConnectItBoardOperation_MovePiece> Moves = MovesFrom(Effect, Board, FGridPosition(2, 2), Seed);
            bAlwaysTheOne &= Moves.Num() == 1 && Moves[0].From == FGridPosition(2, 3) && Moves[0].To == FGridPosition(2, 4);
        }
        TestTrue(TEXT("push: the only valid push, every time, and never its own piece"), bAlwaysTheOne);

        Board.GetTileDataMutable(FGridPosition(2, 4))->bIsActive = false;
        TestEqual(TEXT("push: nothing valid -- nothing to do"), MovesFrom(Effect, Board, FGridPosition(2, 2), 1).Num(), 0);

        FConnectItBoardState Two = MakeBoard(5);
        Place(Two, 2, 3, 1);
        Place(Two, 3, 2, 1);
        const TArray<FConnectItBoardOperation_MovePiece> First = MovesFrom(Effect, Two, FGridPosition(2, 2), 7);
        const TArray<FConnectItBoardOperation_MovePiece> Second = MovesFrom(Effect, Two, FGridPosition(2, 2), 7);
        TestTrue(TEXT("push: two options, the same seed picks the same"),
            First.Num() == 1 && Second.Num() == 1 && First[0].From == Second[0].From);
    }

    // --- Push in the chain: the pushed piece completes a line ---
    {
        FConnectItBoardState Board = MakeBoard(6);
        Place(Board, 0, 0, 0);
        Place(Board, 1, 0, 0);
        Place(Board, 2, 0, 0);
        Board.GetTileDataMutable(FGridPosition(1, 0))->TilePowerTag = ConnectIt_TilePower_Push;
        Place(Board, 1, 1, 1);   // pushed down to (1,2)...
        Place(Board, 0, 2, 1);   // ...completing (0..3, 2) for faction 1
        Place(Board, 2, 2, 1);
        Place(Board, 3, 2, 1);

        const FConnectItBoardState Before = Board;
        FConnectItBoardChangeEvent Events;
        Rules.CarryOutOperation(MakeAt<FConnectItBoardOperation_PlacePiece>(0, 3, 0), Board, &Events);
        TestTrue(TEXT("push chain: both lines scored"), Board.GetScore(0) == 4.f && Board.GetScore(1) == 4.f);
        TestTrue(TEXT("push chain: Power Triggered, then Piece Moved"), PowersTriggered(Events).Num() == 1
            && Events.Events.ContainsByPredicate([](const FInstancedStruct& Entry) { return Entry.GetPtr<FConnectItBoardEvent_PieceMoved>() != nullptr; }));
        TestTrue(TEXT("push chain: before + events = after"), ReplaysTo(Before, Events, Board));
    }

    // --- Vacuum: the nearest piece per direction, into its empty tile ---
    {
        const FConnectItTilePowerEffect_Vacuum Effect;
        FConnectItBoardState Board = MakeBoard(5);
        Place(Board, 2, 4, 0);                                         // down, over the empty (2,3): candidate
        Board.GetTileDataMutable(FGridPosition(3, 2))->bIsActive = false;
        Place(Board, 4, 2, 1);                                         // right, behind an inactive tile: no
        Place(Board, 1, 1, 1);                                         // up-left, locked: no
        Board.GetTileDataMutable(FGridPosition(1, 1))->bIsLocked = true;
        bool bAlwaysTheOne = true;
        for (int32 Seed = 1; Seed <= 8; Seed++)
        {
            const TArray<FConnectItBoardOperation_MovePiece> Moves = MovesFrom(Effect, Board, FGridPosition(2, 2), Seed);
            bAlwaysTheOne &= Moves.Num() == 1 && Moves[0].From == FGridPosition(2, 4) && Moves[0].To == FGridPosition(2, 2);
        }
        TestTrue(TEXT("vacuum: the nearest movable piece, past empty tiles, stopped by inactive ones"), bAlwaysTheOne);

        Place(Board, 2, 2, 1);
        TestEqual(TEXT("vacuum: not into an occupied tile"), MovesFrom(Effect, Board, FGridPosition(2, 2), 1).Num(), 0);
    }

    // --- Vacuum fires on a Piece Exit... ---
    {
        FConnectItBoardState Board = MakeBoard(5);
        Board.GetTileDataMutable(FGridPosition(2, 2))->TilePowerTag = ConnectIt_TilePower_Vacuum;
        Place(Board, 2, 2, 1);
        Place(Board, 4, 4, 0);
        const FConnectItBoardState Before = Board;
        FConnectItBoardChangeEvent Events;
        Rules.CarryOutOperation(MakeAt<FConnectItBoardOperation_RemovePiece>(0, 2, 2), Board, &Events);
        TestTrue(TEXT("vacuum exit: the piece left, another was pulled in"),
            TileAt(Board, 2, 2).FactionPiece == 0 && !Board.IsTileOccupied(FGridPosition(4, 4)));
        TestTrue(TEXT("vacuum exit: before + events = after"), ReplaysTo(Before, Events, Board));
    }

    // --- ...and on Scoring, when the line cleared its piece ---
    {
        FConnectItBoardState Board = MakeBoard(6);
        Board.GetTileDataMutable(FGridPosition(1, 0))->TilePowerTag = ConnectIt_TilePower_Vacuum;
        Place(Board, 0, 0, 0);
        Place(Board, 1, 0, 0);
        Place(Board, 2, 0, 0);
        Place(Board, 1, 5, 1);   // straight down the column, over empty tiles
        FConnectItBoardChangeEvent Events;
        Rules.CarryOutOperation(MakeAt<FConnectItBoardOperation_PlacePiece>(0, 3, 0), Board, &Events);
        // Two candidates: (1,5) down the column, and the line's kept piece at (3,0) along the row
        TestTrue(TEXT("vacuum scoring: its piece cleared by the line, a piece pulled in from a candidate"),
            Board.IsTileOccupied(FGridPosition(1, 0))
            && (!Board.IsTileOccupied(FGridPosition(1, 5)) || !Board.IsTileOccupied(FGridPosition(3, 0))));
        TestEqual(TEXT("vacuum scoring: fired once (Scoring), not again for the Exit it caused"), PowersTriggered(Events).Num(), 1);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItTurnStartTilePowersTest,
    "ConnectIt.Board.TurnStartTilePowers", TestFlags)

bool FConnectItTurnStartTilePowersTest::RunTest(const FString& Parameters)
{
    // First tile powers plan, Stage 3: Turn Start powers (a countdown) and Repel
    FConnectItTilePower Repel = MakeTestPower(ConnectIt_TilePower_Repel, EConnectItTilePowerTrigger::TurnStart,
        FConnectItTilePowerEffect_Repel(), /*CooldownTurns=*/3);
    FConnectItRuleSet Rules;
    Rules.SetTilePowers({ Repel }, {});
    const FConnectItBoardOperation_TurnStart TurnStart;

    const auto StartTurn = [&Rules, &TurnStart](FConnectItBoardState& Board, int32 Turn)
    {
        Board.TurnNumber = Turn;
        FConnectItBoardChangeEvent Events;
        if (Rules.HasTurnStartTilePowerReady(Board, Turn))
        {
            FConnectItBoardOperation_TurnStart Operation = TurnStart;
            Operation.Faction = Turn % 2;
            Rules.CarryOutOperation(Operation, Board, &Events);
        }
        return Events;
    };
    const auto ReplaysTo = [](const FConnectItBoardState& Before, const FConnectItBoardChangeEvent& Events, const FConnectItBoardState& After)
    {
        FConnectItBoardState Replayed = Before;
        FConnectItBoardEvent::ApplyAll(Events, Replayed);
        return Replayed.HasSameTilesAndScores(After);
    };

    // --- The countdown: every 3 turns, reset even with no piece ---
    {
        FConnectItBoardState Board = MakeBoard(5);
        FConnectItTileData* Tile = Board.GetTileDataMutable(FGridPosition(2, 2));
        Tile->TilePowerTag = ConnectIt_TilePower_Repel;
        Tile->TilePowerReadyTurn = 3;   // built at turn 0
        Place(Board, 2, 2, 0);

        TArray<int32> FiredOn;
        bool bReplays = true;
        for (int32 Turn = 1; Turn <= 7; Turn++)
        {
            const FConnectItBoardState Before = Board;
            const FConnectItBoardChangeEvent Events = StartTurn(Board, Turn);
            if (!PowersTriggered(Events).IsEmpty()) FiredOn.Add(Turn);
            bReplays &= ReplaysTo(Before, Events, Board);
        }
        TestTrue(TEXT("countdown: fires on turns 3 and 6"), FiredOn == TArray<int32>({ 3, 6 }));
        TestFalse(TEXT("countdown: the first time it moved the piece off"), Board.IsTileOccupied(FGridPosition(2, 2)));
        TestEqual(TEXT("countdown: the second (no piece) still reset it"), TileAt(Board, 2, 2).TilePowerReadyTurn, 9);
        TestTrue(TEXT("countdown: before + events = after, each turn"), bReplays);
        TestFalse(TEXT("countdown: nothing ready -- no turn-start change to make"), Rules.HasTurnStartTilePowerReady(Board, 8));
        TestFalse(TEXT("countdown: no Turn Start powers -- none"), FConnectItRuleSet().HasTurnStartTilePowerReady(Board, 9));
    }

    // --- The countdown starts when the tile joins the board ---
    {
        FConnectItBoardState Board = MakeBoard(3);
        Board.TurnNumber = 5;
        FConnectItBoardOperation_AddTiles Add;
        Add.Faction = INDEX_NONE;
        FConnectItBoardLayoutCell& Cell = Add.Cells.AddDefaulted_GetRef();
        Cell.Position = FGridPosition(3, 0);
        Cell.TileData.TilePowerTag = ConnectIt_TilePower_Repel;
        const FConnectItBoardState Before = Board;
        FConnectItBoardChangeEvent Events;
        Rules.CarryOutOperation(Add, Board, &Events);
        TestEqual(TEXT("join: added at turn 5, first fires at turn 8"), TileAt(Board, 3, 0).TilePowerReadyTurn, 8);
        TestTrue(TEXT("join: before + events = after"), ReplaysTo(Before, Events, Board));

        UConnectIt_TileCatalogDataAsset* Catalog = NewObject<UConnectIt_TileCatalogDataAsset>();
        FConnectItTileCatalogEntry Entry;
        Entry.TilePower = Repel;
        Catalog->Tiles.Add(ConnectIt_TilePower_Repel, Entry);
        AConnectIt_BuilderTile* Builder = GetMutableDefault<AConnectIt_BuilderTile>();
        const FConnectItTileInitialData Saved = Builder->TileData;
        ON_SCOPE_EXIT { Builder->TileData = Saved; };
        Builder->TileData.TilePowerTag = ConnectIt_TilePower_Repel;
        TestEqual(TEXT("join: built with the board, first fires at turn 3"),
            UConnectIt_BoardStateComponent::MakeTileData(Builder, 2, 1.f, Catalog).TilePowerReadyTurn, 3);
    }

    // --- Repel: only to an open neighbour; boxed in, it stays ---
    {
        FConnectItBoardState Board = MakeBoard(3);
        Board.GetTileDataMutable(FGridPosition(1, 1))->TilePowerTag = ConnectIt_TilePower_Repel;
        for (int32 X = 0; X < 3; X++)
        {
            for (int32 Y = 0; Y < 3; Y++)
            {
                Place(Board, X, Y, (X + Y) % 2);
            }
        }
        Place(Board, 0, 0, -1);
        Board.GetTileDataMutable(FGridPosition(0, 0))->bIsLocked = true;   // open but locked
        Place(Board, 2, 2, -1);
        Board.GetTileDataMutable(FGridPosition(2, 2))->bIsActive = false;  // open but inactive

        FConnectItBoardState Boxed = Board;
        const FConnectItBoardChangeEvent Events = StartTurn(Boxed, 1);
        const auto Triggered = PowersTriggered(Events);
        TestTrue(TEXT("repel: boxed in (locked, inactive) -- fires, does nothing"),
            Triggered.Num() == 1 && Triggered[0]->bNoEffect && TileAt(Boxed, 1, 1).FactionPiece == 0);

        Place(Board, 2, 1, -1);   // one open neighbour
        bool bAlwaysThere = true;
        for (int32 Seed = 1; Seed <= 6; Seed++)
        {
            FConnectItBoardState Seeded = Board;
            Seeded.RandomSeed = Seed;
            StartTurn(Seeded, 1);
            bAlwaysThere &= TileAt(Seeded, 2, 1).FactionPiece == 0 && !Seeded.IsTileOccupied(FGridPosition(1, 1));
        }
        TestTrue(TEXT("repel: to the one open tile, every seed"), bAlwaysThere);
    }

    // --- A repelled piece can score ---
    {
        FConnectItBoardState Board = MakeBoard(5);
        Place(Board, 1, 0, 0);
        Place(Board, 2, 0, 0);
        Place(Board, 3, 0, 0);
        Board.GetTileDataMutable(FGridPosition(0, 1))->TilePowerTag = ConnectIt_TilePower_Repel;
        Place(Board, 0, 1, 0);   // repelled to (0,0), the only open neighbour
        Place(Board, 1, 1, 1);
        Place(Board, 0, 2, 1);
        Place(Board, 1, 2, 1);
        StartTurn(Board, 3);
        TestEqual(TEXT("scoring: the repelled piece completed (0..3, 0)"), Board.GetScore(0), 4.f);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItTileHooksTest,
    "ConnectIt.Board.TileHooks", TestFlags)

bool FConnectItTileHooksTest::RunTest(const FString& Parameters)
{
    // First tile powers plan, Stage 4: what runs without a world -- the
    // reaction handle, the hooks' defaults, finishing a tile's held reactions
    int32 Finished = 0;
    const auto MakeReaction = [&Finished]()
    {
        UConnectIt_TileReaction* Reaction = NewObject<UConnectIt_TileReaction>();
        Reaction->OnFinished = [&Finished]() { Finished++; };
        return Reaction;
    };

    // --- A reaction finishes once ---
    {
        UConnectIt_TileReaction* Reaction = MakeReaction();
        TestFalse(TEXT("reaction: not finished to start"), Reaction->IsFinished());
        Reaction->Finish();
        Reaction->Finish();
        TestTrue(TEXT("reaction: finished, counted once"), Reaction->IsFinished() && Finished == 1);
    }

    // --- Each hook's default finishes at once ---
    {
        AConnectIt_TileBase* Tile = GetMutableDefault<AConnectIt_TileBase>();
        Finished = 0;
        Tile->OnTilePowerTriggered(FConnectItBoardEvent_TilePowerTriggered(), MakeReaction());
        Tile->OnPieceEnter(0, ConnectIt_Event_PiecePlaced, MakeReaction());
        Tile->OnPieceExit(0, ConnectIt_Event_PieceRemoved, MakeReaction());
        Tile->OnMultiplierChanged(2.f, MakeReaction());
        Tile->OnLockChanged(true, MakeReaction());
        Tile->OnActiveChanged(false, MakeReaction());
        Tile->OnChainPartnerChanged(FConnectItBoardEvent_ChainPartnerChanged(), MakeReaction());
        TestEqual(TEXT("hooks: all seven defaults finished their reaction"), Finished, 7);
    }

    // --- A tile leaving play finishes what it still holds ---
    {
        AConnectIt_TileBase* Tile = GetMutableDefault<AConnectIt_TileBase>();
        Finished = 0;
        UConnectIt_TileReaction* First = MakeReaction();
        UConnectIt_TileReaction* Second = MakeReaction();
        Tile->HoldReaction(First);
        Tile->HoldReaction(Second);
        Second->Finish();
        Tile->FinishPendingReactions();
        TestTrue(TEXT("pending: both finished, each counted once"), First->IsFinished() && Second->IsFinished() && Finished == 2);
    }

    // --- The visualizer handles the tile events ---
    {
        const TArray<FGameplayTag> Handled = UConnectIt_TileVisualizer::GetHandledEventTags();
        TestTrue(TEXT("visualizer: power, pieces, multipliers, lock, active, chain"),
            Handled.Contains(FGameplayTag(ConnectIt_Event_TilePowerTriggered)) && Handled.Contains(FGameplayTag(ConnectIt_Event_PieceMoved))
            && Handled.Contains(FGameplayTag(ConnectIt_Event_Scored)) && Handled.Contains(FGameplayTag(ConnectIt_Event_TileLockChanged))
            && Handled.Contains(FGameplayTag(ConnectIt_Event_ChainPartnerChanged)) && Handled.Num() == 12);
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
