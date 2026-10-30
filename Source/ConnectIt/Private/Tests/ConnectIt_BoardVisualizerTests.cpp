// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "ConnectIt_BoardVisualizerTestTypes.h"
#include "Board/ConnectIt_BoardVisualizer.h"
#include "Board/ConnectIt_PieceRegistry.h"
#include "Board/ConnectIt_TileRegistry.h"
#include "Framework/Subsystem/ConnectIt_BoardRegistrySubsystem.h"
#include "Registry/GridDefinition.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Board/ConnectIt_VisualHandle.h"
#include "Board/Actor/ConnectIt_PieceBase.h"
#include "Board/Actor/ConnectIt_TileBase.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "ConnectIt_GameplayTags.h"
#include "GameEvent/GameEventTaskSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "TimerManager.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ConnectItVisualizerTests
{
    constexpr EAutomationTestFlags TestFlags =
        EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

    // A throwaway game world (timers, the event queue, spawning)
    struct FTestWorld
    {
        UWorld* World = nullptr;

        FTestWorld()
        {
            World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("ConnectItVisualizerTestWorld"));
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        }

        ~FTestWorld()
        {
            GEngine->DestroyWorldContext(World);
            World->DestroyWorld(false);
        }

        // Runs the world's timers for Frames frames of Seconds each. The timer
        // manager ticks once per engine frame (a timer set before its first
        // tick only becomes active at the end of one), so each step moves the
        // frame counter on as the engine would.
        void TickTimers(float Seconds, int32 Frames = 3) const
        {
            for (int32 Frame = 0; Frame < Frames; Frame++)
            {
                World->GetTimerManager().Tick(Seconds);
                GFrameCounter++;
            }
        }
    };

    UConnectIt_VisualHandle* MakeHandle(int32& Completions, UObject* Outer = GetTransientPackage())
    {
        UConnectIt_VisualHandle* Handle = NewObject<UConnectIt_VisualHandle>(Outer);
        Handle->OnComplete.AddLambda([&Completions](UConnectIt_VisualHandle*) { Completions++; });
        Handle->Begin(FGameplayTag(ConnectIt_Event_PiecePlaced), 0.f);
        return Handle;
    }
}

// Board visualizer plan, Stage 2: the completion handle
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItVisualHandleTest,
    "ConnectIt.Visuals.Handle", ConnectItVisualizerTests::TestFlags)

bool FConnectItVisualHandleTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItVisualizerTests;
    UObject* HolderA = NewObject<UConnectIt_VisualHandle>();
    UObject* HolderB = NewObject<UConnectIt_VisualHandle>();

    // --- Nothing holds it: complete as soon as it is sealed ---
    {
        int32 Completions = 0;
        UConnectIt_VisualHandle* Handle = MakeHandle(Completions);
        TestFalse(TEXT("no holds: open until sealed"), Handle->IsComplete());
        Handle->Seal();
        TestTrue(TEXT("no holds: complete at once, once"), Handle->IsComplete() && Completions == 1);
    }

    // --- Two holds: complete after both releases; a stray release changes nothing ---
    {
        int32 Completions = 0;
        UConnectIt_VisualHandle* Handle = MakeHandle(Completions);
        Handle->Hold(HolderA);
        Handle->Hold(HolderB);
        Handle->Seal();
        TestFalse(TEXT("two holds: still playing"), Handle->IsComplete());
        Handle->Release(HolderA);
        Handle->Release(HolderA);   // A holds nothing now: ignored
        TestTrue(TEXT("two holds: one left after a double release"), !Handle->IsComplete() && Handle->GetHoldCount() == 1);
        Handle->Release(HolderB);
        TestTrue(TEXT("two holds: complete after both, once"), Handle->IsComplete() && Completions == 1);

        Handle->Hold(HolderA);
        Handle->Release(HolderA);
        TestTrue(TEXT("after completion: holds and releases ignored"), Handle->GetHoldCount() == 0 && Completions == 1);
    }

    // --- An actor leaving play lets go of all its holds ---
    {
        int32 Completions = 0;
        UConnectIt_VisualHandle* Handle = MakeHandle(Completions);
        Handle->Hold(HolderA);
        Handle->Hold(HolderA);
        Handle->Seal();
        Handle->ReleaseAllBy(HolderA);
        TestTrue(TEXT("release all by: complete"), Handle->IsComplete() && Completions == 1);
    }

    // --- A hold never released: the timeout completes it (and logs) ---
    {
        const FTestWorld TestWorld;
        int32 Completions = 0;
        UConnectIt_VisualHandle* Handle = NewObject<UConnectIt_VisualHandle>(TestWorld.World);
        Handle->OnComplete.AddLambda([&Completions](UConnectIt_VisualHandle*) { Completions++; });
        Handle->Begin(FGameplayTag(ConnectIt_Event_PiecePlaced), 1.f);
        Handle->Hold(HolderA);
        Handle->Seal();
        AddExpectedError(TEXT("timed out"), EAutomationExpectedErrorFlags::Contains, 1);
        TestWorld.TickTimers(1.f);
        TestTrue(TEXT("timeout: complete, once"), Handle->IsComplete() && Completions == 1);
    }
    return true;
}

// Board visualizer plan, Stage 3: every board event is bound, and an event
// with nothing holding it completes its queue entry
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItBoardVisualizerEventsTest,
    "ConnectIt.Visuals.Events", ConnectItVisualizerTests::TestFlags)

bool FConnectItBoardVisualizerEventsTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItVisualizerTests;

    // --- Every event struct has a tag of its own ---
    const TArray<const UScriptStruct*>& Structs = AConnectIt_BoardVisualizer::GetAllEventStructs();
    TSet<FGameplayTag> Tags;
    for (const UScriptStruct* Struct : Structs)
    {
        FInstancedStruct Instance;
        Instance.InitializeAs(Struct);
        const FGameplayTag Tag = Instance.Get<FConnectItBoardEvent>().GetEventTag();
        TestTrue(FString::Printf(TEXT("events: %s has a tag"), *Struct->GetName()), Tag.IsValid());
        Tags.Add(Tag);
    }
    TestEqual(TEXT("events: each its own tag"), Tags.Num(), Structs.Num());
    TestEqual(TEXT("events: 22 kinds (a new one also needs its visualizer function)"), Structs.Num(), 22);

    // --- One played through the queue: nothing holds it, so its entry completes ---
    {
        const FTestWorld TestWorld;
        // Actors only receive Blueprint-callable calls (the queue's task
        // delegate) once the world's actors are initialised for play
        TestWorld.World->InitializeActorsForPlay(FURL());
        UGameEventTaskSubsystem* Queue = TestWorld.World->GetSubsystem<UGameEventTaskSubsystem>();
        AConnectIt_BoardVisualizer* Visualizer = TestWorld.World->SpawnActor<AConnectIt_BoardVisualizer>();
        if (!TestNotNull(TEXT("queue"), Queue) || !TestNotNull(TEXT("visualizer"), Visualizer)) return false;
        Visualizer->Initialise();

        FConnectItBoardEvent_PhaseChanged Phase;
        Queue->QueueTagContainerWithPayload(FGameplayTagContainer(Phase.GetEventTag()), FInstancedStruct::Make(Phase));
        TestTrue(TEXT("playing: waits a tick (the queue is walking its tasks)"), Queue->GetActivePayload().IsValid());
        TestWorld.TickTimers(0.1f);
        TestFalse(TEXT("played: the entry completed"), Queue->GetActivePayload().IsValid());
    }
    return true;
}

namespace ConnectItVisualizerTests
{
    // A board in a test world: tiles in a row at (0,0), (1,0)... (100 units
    // apart), the registries and the visualizer, with nothing else (no board
    // state, no level config)
    struct FBoardFixture : FTestWorld
    {
        UGameEventTaskSubsystem* Queue = nullptr;
        UConnectIt_TileRegistry* Tiles = nullptr;
        UConnectIt_PieceRegistry* Pieces = nullptr;
        AConnectIt_BoardVisualizer* Visualizer = nullptr;
        TArray<AConnectIt_TestTile*> TileActors;

        explicit FBoardFixture(int32 NumTiles = 3)
        {
            World->InitializeActorsForPlay(FURL());
            UConnectIt_BoardRegistrySubsystem* Registries = World->GetSubsystem<UConnectIt_BoardRegistrySubsystem>();
            UGridDefinition* Grid = NewObject<UGridDefinition>(Registries);
            Grid->GridSize = 100;
            for (int32 Index = 0; Index < NumTiles; Index++)
            {
                TileActors.Add(World->SpawnActor<AConnectIt_TestTile>(FVector(Index * 100.f, 0.f, 0.f), FRotator::ZeroRotator));
            }
            Tiles = NewObject<UConnectIt_TileRegistry>(Registries);
            Tiles->GridDefinition = Grid;
            Pieces = NewObject<UConnectIt_PieceRegistry>(Registries);
            Pieces->GridDefinition = Grid;
            Visualizer = World->SpawnActor<AConnectIt_BoardVisualizer>();
            Registries->SetUpForTest(Tiles, Pieces, Grid, Visualizer);
            Tiles->InitialiseRegistry();
            Visualizer->Initialise();
            Queue = World->GetSubsystem<UGameEventTaskSubsystem>();
        }

        ~FBoardFixture()
        {
            Tiles->ShutdownRegistry();
        }

        // A piece of Faction at Position, on its tile and mapped
        AConnectIt_TestPiece* AddPiece(const FGridPosition& Position, int32 Faction) const
        {
            AConnectIt_TestPiece* Piece = World->SpawnActor<AConnectIt_TestPiece>(
                Visualizer->GetPieceLocation(Position), FRotator::ZeroRotator);
            Piece->InitialisePiece(Faction);
            Pieces->MapPiece(Position, Piece);
            return Piece;
        }

        // Plays Event through the queue and the visualizer
        template <typename TEvent>
        void Play(const TEvent& Event) const
        {
            Queue->QueueTagContainerWithPayload(FGameplayTagContainer(Event.GetEventTag()), FInstancedStruct::Make(Event));
            TickTimers(0.1f);
        }

        bool IsIdle() const { return !Queue->GetActivePayload().IsValid(); }
    };

    // Sets ConnectIt.Visualizer.Reconcile for a scope
    struct FReconcileMode
    {
        IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("ConnectIt.Visualizer.Reconcile"));
        int32 Previous = Variable ? Variable->GetInt() : 2;
        explicit FReconcileMode(int32 Mode) { if (Variable) Variable->Set(Mode, ECVF_SetByCode); }
        ~FReconcileMode() { if (Variable) Variable->Set(Previous, ECVF_SetByCode); }
    };
}

// Board visualizer revision, Stage 1: every mapping sets the piece's position
// (map, move, swap, shift; unmapping leaves it as it was)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItPiecePositionCacheTest,
    "ConnectIt.Board.PiecePositionCache", ConnectItVisualizerTests::TestFlags)

bool FConnectItPiecePositionCacheTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItVisualizerTests;
    const FBoardFixture Board;
    UConnectIt_PieceRegistry* Pieces = Board.Pieces;
    const auto Agrees = [Pieces](const AGridPieceBase* Piece, const FGridPosition& Expected)
    {
        FGridPosition Mapped;
        return Pieces->FindPositionOfPiece(Piece, Mapped) && Mapped == Expected && Piece->GetGridPosition() == Expected;
    };

    AConnectIt_TestPiece* A = Board.AddPiece(FGridPosition(0, 0), 0);
    AConnectIt_TestPiece* B = Board.AddPiece(FGridPosition(1, 0), 1);
    TestTrue(TEXT("map: A at (0,0)"), Agrees(A, FGridPosition(0, 0)));
    TestTrue(TEXT("map: B at (1,0)"), Agrees(B, FGridPosition(1, 0)));

    Pieces->MovePieceActor(FGridPosition(0, 0), FGridPosition(2, 0));
    TestTrue(TEXT("move: A at (2,0)"), Agrees(A, FGridPosition(2, 0)));

    Pieces->SwapPieceActors(FGridPosition(1, 0), FGridPosition(2, 0));
    TestTrue(TEXT("swap: A at (1,0)"), Agrees(A, FGridPosition(1, 0)));
    TestTrue(TEXT("swap: B at (2,0)"), Agrees(B, FGridPosition(2, 0)));

    Pieces->SwapPieceActors(FGridPosition(0, 0), FGridPosition(1, 0));
    TestTrue(TEXT("swap with an empty cell: A at (0,0)"), Agrees(A, FGridPosition(0, 0)));

    // A rotation: (0,0) -> (1,0) -> (2,0) -> (0,0)
    Pieces->ShiftPieceActors({ FGridPosition(0, 0), FGridPosition(1, 0), FGridPosition(2, 0) },
        { FGridPosition(1, 0), FGridPosition(2, 0), FGridPosition(0, 0) });
    TestTrue(TEXT("shift: A at (1,0)"), Agrees(A, FGridPosition(1, 0)));
    TestTrue(TEXT("shift: B wrapped to (0,0)"), Agrees(B, FGridPosition(0, 0)));

    Pieces->UnmapPiece(FGridPosition(1, 0));
    TestFalse(TEXT("unmap: A no longer mapped"), Pieces->GetPieceAtPosition(FGridPosition(1, 0)) == A);
    TestTrue(TEXT("unmap: A keeps its last position (for its leaving visuals)"), A->GetGridPosition() == FGridPosition(1, 0));
    return true;
}

// Board visualizer revision, Stage 2: each default hands its hooks a copy of
// the event it was fired by (or one made for it)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItBoardVisualizerHooksTest,
    "ConnectIt.Visuals.HookDefaults", ConnectItVisualizerTests::TestFlags)

bool FConnectItBoardVisualizerHooksTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItVisualizerTests;
    const FBoardFixture Board;
    AConnectIt_TestTile* Tile0 = Board.TileActors[0];
    AConnectIt_TestTile* Tile1 = Board.TileActors[1];
    AConnectIt_TestPiece* P = Board.AddPiece(FGridPosition(0, 0), 0);

    // --- Placed: the piece and its tile get the event ---
    FConnectItBoardEvent_PiecePlaced Placed;
    Placed.Position = FGridPosition(0, 0);
    Placed.Faction = 0;
    Board.Play(Placed);
    TestTrue(TEXT("placed: the piece got the event"), P->Placed.Num() == 1 && P->Placed[0].Position == FGridPosition(0, 0));
    TestTrue(TEXT("placed: the tile's enter got it"), Tile0->EnterTags.Num() == 1 && Tile0->EnterTags[0] == Placed.GetEventTag());
    TestTrue(TEXT("placed: played out"), Board.IsIdle());

    // --- Moved: exit, enter, the piece's own copy; its position follows ---
    FConnectItBoardEvent_PieceMoved Moved;
    Moved.From = FGridPosition(0, 0);
    Moved.To = FGridPosition(1, 0);
    Moved.Faction = 0;
    Board.Play(Moved);
    TestTrue(TEXT("moved: the piece got From / To"), P->Moved.Num() == 1
        && P->Moved[0].From == FGridPosition(0, 0) && P->Moved[0].To == FGridPosition(1, 0));
    TestTrue(TEXT("moved: exit and enter"), Tile0->ExitTags.Num() == 1 && Tile1->EnterTags.Num() == 1
        && Tile1->EnterTags[0] == Moved.GetEventTag());
    TestTrue(TEXT("moved: the piece's position is (1,0)"), P->GetGridPosition() == FGridPosition(1, 0));

    // --- Captured ---
    FConnectItBoardEvent_PieceCaptured Captured;
    Captured.Position = FGridPosition(1, 0);
    Captured.CapturingFaction = 1;
    Captured.PreviousFaction = 0;
    Board.Play(Captured);
    TestTrue(TEXT("captured: the piece got the new faction"), P->Captured.Num() == 1 && P->Captured[0].CapturingFaction == 1);
    TestTrue(TEXT("captured: the tile's enter has the new faction"), Tile1->EnterFactions.Last() == 1);

    // --- Swapped: both pieces get the event; positions follow ---
    AConnectIt_TestPiece* Q = Board.AddPiece(FGridPosition(2, 0), 0);
    FConnectItBoardEvent_PiecesSwapped Swapped;
    Swapped.PositionA = FGridPosition(1, 0);
    Swapped.PositionB = FGridPosition(2, 0);
    Board.Play(Swapped);
    TestTrue(TEXT("swapped: both pieces got it"), P->Swapped.Num() == 1 && Q->Swapped.Num() == 1);
    TestTrue(TEXT("swapped: positions traded"), P->GetGridPosition() == FGridPosition(2, 0) && Q->GetGridPosition() == FGridPosition(1, 0));

    // --- Shifted: each riding piece gets a Piece Moved made for it ---
    FConnectItBoardEvent_BoardShifted Shifted;
    Shifted.StartPositions = { FGridPosition(0, 0), FGridPosition(1, 0), FGridPosition(2, 0) };
    Shifted.EndPositions = { FGridPosition(1, 0), FGridPosition(2, 0), FGridPosition(0, 0) };
    Board.Play(Shifted);
    TestTrue(TEXT("shifted: Q got a Piece Moved (1,0) -> (2,0)"), Q->Moved.Num() == 1
        && Q->Moved[0].From == FGridPosition(1, 0) && Q->Moved[0].To == FGridPosition(2, 0));
    TestTrue(TEXT("shifted: P wrapped to (0,0)"), P->GetGridPosition() == FGridPosition(0, 0));
    TestEqual(TEXT("shifted: tiles and pieces on their cells"), Board.Visualizer->ReconcileAll(), 0);

    // --- Multipliers: the event itself, or one made for the tile ---
    FConnectItBoardEvent_MultiplierChanged Adjusted;
    Adjusted.Position = FGridPosition(0, 0);
    Adjusted.NewMultiplier = 2.f;
    Board.Play(Adjusted);
    AConnectIt_TestTile* AtOrigin = Cast<AConnectIt_TestTile>(Board.Tiles->GetTileAt(FGridPosition(0, 0)));
    TestTrue(TEXT("multiplier: the tile got it"), AtOrigin && AtOrigin->Multipliers.Num() == 1 && AtOrigin->Multipliers[0].NewMultiplier == 2.f);

    FConnectItBoardEvent_TileMultiplierDestroyed Destroyed;
    Destroyed.Position = FGridPosition(0, 0);
    Destroyed.PreviousMultiplier = 3.f;
    Destroyed.NewMultiplier = 1.f;
    Board.Play(Destroyed);
    TestTrue(TEXT("destroyed: a Multiplier Changed made for it, 3 -> 1"), AtOrigin && AtOrigin->Multipliers.Num() == 2
        && AtOrigin->Multipliers[1].PreviousMultiplier == 3.f && AtOrigin->Multipliers[1].NewMultiplier == 1.f);

    // --- Scored: the cleared piece gets the event ---
    FConnectItBoardEvent_Scored Scored;
    Scored.ClearedPositions = { FGridPosition(0, 0) };
    Board.Play(Scored);
    TestEqual(TEXT("scored: P cleared"), P->Cleared, 1);
    TestTrue(TEXT("scored: played out"), Board.IsIdle());
    return true;
}

// Board visualizer revision, Stage 3: a piece a visual leaves off its tile is
// reported and put right; a tile mid-hold isn't checked until its handle
// completes; log-only reports without moving
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItBoardVisualizerReconcileTest,
    "ConnectIt.Visuals.Reconcile", ConnectItVisualizerTests::TestFlags)

bool FConnectItBoardVisualizerReconcileTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItVisualizerTests;
    AddExpectedError(TEXT("reconcile --"), EAutomationExpectedErrorFlags::Contains, 3);
    const FBoardFixture Board;
    AConnectIt_TestPiece* P = Board.AddPiece(FGridPosition(0, 0), 0);
    P->bWanderOnCapture = true;
    const FVector Home = Board.Visualizer->GetPieceLocation(FGridPosition(0, 0));

    // --- A visual that forgets: reported once, put back ---
    FConnectItBoardEvent_PieceCaptured Captured;
    Captured.Position = FGridPosition(0, 0);
    Captured.CapturingFaction = 1;
    Board.Play(Captured);
    TestTrue(TEXT("forgetful: the piece is back on its tile"), P->GetActorLocation().Equals(Home, 1.f));

    // --- A tile mid-hold: not checked until its handle completes ---
    AConnectIt_TestTile* Tile1 = Board.TileActors[1];
    Tile1->bHoldOnLock = true;
    FConnectItBoardEvent_TileLockChanged Lock;
    Lock.Position = FGridPosition(1, 0);
    Lock.bLocked = true;
    Board.Play(Lock);
    TestFalse(TEXT("held: still playing"), Board.IsIdle());
    TestFalse(TEXT("held: the tile isn't corrected mid-hold"), Tile1->GetActorLocation().Equals(FVector(100.f, 0.f, 0.f), 1.f));
    Tile1->ReleaseHeld();
    Board.TickTimers(0.1f);
    TestTrue(TEXT("released: played out"), Board.IsIdle());
    TestTrue(TEXT("released: the tile is back on its cell"), Tile1->GetActorLocation().Equals(FVector(100.f, 0.f, 0.f), 1.f));
    TestEqual(TEXT("all on their cells now"), Board.Visualizer->ReconcileAll(), 0);

    // --- Log only: reported, not moved ---
    {
        const FReconcileMode LogOnly(1);
        Board.Play(Captured);
        TestFalse(TEXT("log only: the piece stays where it wandered"), P->GetActorLocation().Equals(Home, 1.f));
    }

    // --- Off: nothing checked ---
    {
        const FReconcileMode Off(0);
        TestEqual(TEXT("off: nothing reported"), Board.Visualizer->ReconcileAll(), 0);
    }
    return true;
}

// Board visualizer revision, Stage 4: event visuals by declared tag; one runs
// instead of the default, Play Default runs the default too; the handle still
// completes the event; an event with none plays its default
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItEventVisualsTest,
    "ConnectIt.Visuals.EventVisuals", ConnectItVisualizerTests::TestFlags)

bool FConnectItEventVisualsTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItVisualizerTests;

    // --- The map: by declared tag; a duplicate is an error, empty / abstract warnings ---
    {
        TArray<FString> Errors;
        TArray<FString> Warnings;
        const TMap<FGameplayTag, TSubclassOf<AConnectIt_EventVisual>> Map = AConnectIt_BoardVisualizer::BuildEventVisualMap(
            { AConnectIt_TestPlacedVisual::StaticClass(), AConnectIt_TestPlacedVisualWithDefault::StaticClass(), nullptr,
              AConnectIt_EventVisual_PiecePlaced::StaticClass() },
            &Errors, &Warnings);
        const FGameplayTag PlacedTag = FConnectItBoardEvent_PiecePlaced().GetEventTag();
        TestTrue(TEXT("map: Piece Placed -> the first class"), Map.Num() == 1 && Map.FindRef(PlacedTag) == AConnectIt_TestPlacedVisual::StaticClass());
        TestEqual(TEXT("map: the duplicate reported"), Errors.Num(), 1);
        TestEqual(TEXT("map: the empty and the abstract entries warned"), Warnings.Num(), 2);
    }

    const auto FindVisual = [](UWorld* World) -> AConnectIt_TestPlacedVisual*
    {
        TActorIterator<AConnectIt_TestPlacedVisual> It(World);
        return It ? *It : nullptr;
    };

    FConnectItBoardEvent_PiecePlaced Placed;
    Placed.Position = FGridPosition(0, 0);
    Placed.Faction = 0;

    // --- An override runs instead of the default ---
    {
        const FBoardFixture Board;
        AConnectIt_TestPiece* P = Board.AddPiece(FGridPosition(0, 0), 0);
        Board.Visualizer->SetEventVisuals({ AConnectIt_TestPlacedVisual::StaticClass() });
        Board.Play(Placed);
        AConnectIt_TestPlacedVisual* Visual = FindVisual(Board.World);
        TestTrue(TEXT("override: played, with the event"), Visual && Visual->Plays == 1 && Visual->LastEvent.Position == FGridPosition(0, 0));
        TestEqual(TEXT("override: the default hooks didn't run"), P->Placed.Num(), 0);
        TestTrue(TEXT("override: the event completed"), Board.IsIdle());

        Board.Play(Placed);
        TestTrue(TEXT("override: spawned once, reused"), Visual && Visual->Plays == 2);

        // An event with no visual plays its default
        FConnectItBoardEvent_PieceCaptured Captured;
        Captured.Position = FGridPosition(0, 0);
        Captured.CapturingFaction = 1;
        Board.Play(Captured);
        TestEqual(TEXT("no visual: the default ran"), P->Captured.Num(), 1);
    }

    // --- Play Default runs the default too ---
    {
        const FBoardFixture Board;
        AConnectIt_TestPiece* P = Board.AddPiece(FGridPosition(0, 0), 0);
        Board.Visualizer->SetEventVisuals({ AConnectIt_TestPlacedVisualWithDefault::StaticClass() });
        Board.Play(Placed);
        AConnectIt_TestPlacedVisual* Visual = FindVisual(Board.World);
        TestTrue(TEXT("play default: the visual played"), Visual && Visual->Plays == 1);
        TestEqual(TEXT("play default: and the piece's hook"), P->Placed.Num(), 1);
        TestTrue(TEXT("play default: the event completed"), Board.IsIdle());
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
