// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "Board/ConnectIt_BoardVisualizer.h"
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

// Board visualizer plan, Stage 4: the actors' hook defaults hold nothing
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItBoardVisualizerHooksTest,
    "ConnectIt.Visuals.HookDefaults", ConnectItVisualizerTests::TestFlags)

bool FConnectItBoardVisualizerHooksTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItVisualizerTests;
    int32 Completions = 0;
    UConnectIt_VisualHandle* Handle = MakeHandle(Completions);

    AConnectIt_TileBase* Tile = GetMutableDefault<AConnectIt_TileBase>();
    Tile->OnTilePowerTriggered(FConnectItBoardEvent_TilePowerTriggered(), Handle);
    Tile->OnPieceEnter(0, ConnectIt_Event_PiecePlaced, Handle);
    Tile->OnPieceExit(0, ConnectIt_Event_PieceRemoved, Handle);
    Tile->OnMultiplierChanged(2.f, Handle);
    Tile->OnLockChanged(true, Handle);
    Tile->OnActiveChanged(false, Handle);
    Tile->OnChainPartnerChanged(FConnectItBoardEvent_ChainPartnerChanged(), Handle);

    AConnectIt_PieceBase* Piece = GetMutableDefault<AConnectIt_PieceBase>();
    Piece->OnPlaced(Handle);
    Piece->OnMoved(FGridPosition(0, 0), FGridPosition(1, 0), Handle);
    Piece->OnSwapped(FGridPosition(0, 0), FGridPosition(1, 0), Handle);
    Piece->OnCaptured(1, Handle);
    Piece->OnRemoved(Handle);
    Piece->OnClearedByScore(Handle);

    Handle->Seal();
    TestTrue(TEXT("hook defaults: nothing held, the event completes"), Handle->IsComplete() && Completions == 1);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
