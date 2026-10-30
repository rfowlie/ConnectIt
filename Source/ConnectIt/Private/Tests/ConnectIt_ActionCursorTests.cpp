// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "ConnectIt_ActionCursorTestTypes.h"
#include "Action/ConnectIt_PlacePieceAction.h"
#include "Action/ConnectIt_SwapPieceAction.h"
#include "Pooling/ActorPool.h"
#include "Pooling/ActorPoolSubsystem.h"
#include "Subsystem/GridHoverSubsystem.h"
#include "Engine/TargetPoint.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ConnectItActionCursorTests
{
    constexpr EAutomationTestFlags TestFlags =
        EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

    // Cursors of Class out of the pool right now (0 with no pool yet)
    int32 ActiveCursors(UWorld* World, TSubclassOf<AActor> Class)
    {
        const UActorPool* Pool = World->GetSubsystem<UActorPoolSubsystem>()->FindPool(Class);
        return Pool ? Pool->GetActivePoolCount() : 0;
    }
}

// Action cursor plan, Stage 2: an action's cursors come from the pool, the
// follow cursor swaps class per tile, and every exit (cancel, complete,
// force-deactivate) returns every cursor and puts every touched tile back to
// neutral
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItActionCursorsTest,
    "ConnectIt.PlayerAction.Cursors", ConnectItActionCursorTests::TestFlags)

bool FConnectItActionCursorsTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItActionCursorTests;
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("ConnectItActionCursorTestWorld"));
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
    };
    // Actors only take interface (ProcessEvent) calls once the world is initialised for play
    World->InitializeActorsForPlay(FURL());

    UGridHoverSubsystem* Hover = World->GetSubsystem<UGridHoverSubsystem>();
    if (!TestNotNull(TEXT("the hover subsystem"), Hover)) return false;

    const TSubclassOf<AActor> HoverClass = AConnectIt_TestCursor::StaticClass();
    const TSubclassOf<AActor> OtherClass = AConnectIt_TestCursorOther::StaticClass();
    AActor* Owner = World->SpawnActor<AActor>();
    AGridTileBase* TileA = World->SpawnActor<AGridTileBase>(FVector(100.f, 0.f, 0.f), FRotator::ZeroRotator);
    AGridTileBase* TileB = World->SpawnActor<AGridTileBase>(FVector(200.f, 0.f, 0.f), FRotator::ZeroRotator);

    UConnectIt_TestCursorAction* Action = NewObject<UConnectIt_TestCursorAction>(Owner);
    Action->HoverClass = HoverClass;
    Action->OtherClass = OtherClass;
    Action->OtherTile = TileB;

    // Hover A then B, and park a marker on A
    const auto Play = [&]()
    {
        Action->Activate(nullptr);
        Hover->OnGridTileHoverChanged.Broadcast(TileA);
        Hover->OnGridTileHoverChanged.Broadcast(TileB);
        AActor* Marker = Action->AcquireActionCursor(HoverClass);
        Action->ShowCursorAt(Marker, TileA);
        return Marker;
    };
    const auto CheckClean = [&](const TCHAR* Exit)
    {
        TestEqual(*FString::Printf(TEXT("%s: hover-class cursors all back"), Exit), ActiveCursors(World, HoverClass), 0);
        TestEqual(*FString::Printf(TEXT("%s: other-class cursors all back"), Exit), ActiveCursors(World, OtherClass), 0);
        TestTrue(*FString::Printf(TEXT("%s: tiles neutral"), Exit),
            !TileA->GetCursorState().IsValid() && !TileB->GetCursorState().IsValid());
        TestNull(*FString::Printf(TEXT("%s: no hover cursor"), Exit), Action->GetHoverCursor());
    };

    // --- The follow cursor swaps class per tile; a marker stays put ---
    {
        Action->Activate(nullptr);
        Hover->OnGridTileHoverChanged.Broadcast(TileA);
        AConnectIt_TestCursor* First = Cast<AConnectIt_TestCursor>(Action->GetHoverCursor());
        if (!TestNotNull(TEXT("hover A: a cursor"), First)) return false;
        TestTrue(TEXT("hover A: the hover class"), First->GetClass() == HoverClass);
        TestTrue(TEXT("hover A: shown"), First->bShown);
        TestTrue(TEXT("hover A: told the tile, at the offset"), First->LastTile == TileA
            && First->GetActorLocation().Equals(TileA->GetActorLocation() + Action->CursorOffset));
        TestEqual(TEXT("hover A: a new tile reactivates"), First->Reactivations, 1);
        Hover->OnGridTileHoverChanged.Broadcast(TileA);
        TestEqual(TEXT("hover A again: moved, not reactivated"), First->Reactivations, 1);
        TestTrue(TEXT("hover A: A is hovered"), TileA->GetCursorState() == Grid_CursorState_Hovered);

        Hover->OnGridTileHoverChanged.Broadcast(TileB);
        AConnectIt_TestCursor* Second = Cast<AConnectIt_TestCursor>(Action->GetHoverCursor());
        TestTrue(TEXT("hover B: the other class"), Second && Second->GetClass() == OtherClass);
        TestEqual(TEXT("hover B: the old cursor went back"), ActiveCursors(World, HoverClass), 0);
        TestFalse(TEXT("hover B: the old cursor hidden"), First->bShown);

        AConnectIt_TestCursor* Marker = Cast<AConnectIt_TestCursor>(Action->AcquireActionCursor(HoverClass));
        if (!TestNotNull(TEXT("marker: acquired"), Marker)) return false;
        TestFalse(TEXT("marker: hidden until shown"), Marker->bShown);
        Action->ShowCursorAt(Marker, TileA);
        TestTrue(TEXT("marker: shown on A"), Marker->bShown && Marker->LastTile == TileA);

        Action->Cancel();
        CheckClean(TEXT("cancel"));
        TestFalse(TEXT("cancel: the marker hidden"), Marker->bShown);
        TestFalse(TEXT("cancel: the marker no longer held"), Action->HoldsActionCursor(Marker));
    }

    // --- Complete and force-deactivate clean up the same way ---
    Play();
    Action->Complete();
    CheckClean(TEXT("complete"));

    Play();
    Action->ForceDeactivate();
    CheckClean(TEXT("force"));

    // --- A class that isn't a cursor: refused, with an error ---
    AddExpectedError(TEXT("doesn't implement IGridCursor"), EAutomationExpectedErrorFlags::Contains, 1);
    Action->Activate(nullptr);
    TestNull(TEXT("not a cursor: refused"), Action->AcquireActionCursor(ATargetPoint::StaticClass()));
    TestEqual(TEXT("not a cursor: nothing taken from the pool"), ActiveCursors(World, ATargetPoint::StaticClass()), 0);
    Action->Cancel();

    // --- No class: no cursor ---
    Action->HoverClass = nullptr;
    Action->Activate(nullptr);
    Hover->OnGridTileHoverChanged.Broadcast(TileA);
    TestNull(TEXT("no class: no cursor"), Action->GetHoverCursor());
    TestTrue(TEXT("no class: the tile state still set"), TileA->GetCursorState() == Grid_CursorState_Hovered);
    Action->Cancel();
    CheckClean(TEXT("no class"));
    return true;
}

// Action cursor plan, Stage 3: Place and Swap (with their own Deactivate
// paths) hand back cursors and tile states on every exit
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItActionCursorsAdoptedTest,
    "ConnectIt.PlayerAction.CursorsAdopted", ConnectItActionCursorTests::TestFlags)

bool FConnectItActionCursorsAdoptedTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItActionCursorTests;
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("ConnectItActionCursorsAdoptedTestWorld"));
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
    };

    // No board in this world: Swap's deactivate reports its missing tile registry
    AddExpectedError(TEXT("no valid TileRegistry"), EAutomationExpectedErrorFlags::Contains, 0);

    const TSubclassOf<AActor> CursorClass = AConnectIt_TestCursor::StaticClass();
    AActor* Owner = World->SpawnActor<AActor>();
    AGridTileBase* Tile = World->SpawnActor<AGridTileBase>();

    UConnectIt_PlacePieceAction* Place = NewObject<UConnectIt_PlacePieceAction>(Owner);
    UConnectIt_SwapPieceAction* Swap = NewObject<UConnectIt_SwapPieceAction>(Owner);
    TestTrue(TEXT("Place: hovered by default"), Place->TagActionMouseHover == Grid_CursorState_Hovered);
    TestTrue(TEXT("Swap: first pick selected by default"), Swap->TagFirstSelection == Grid_CursorState_Selected);

    const auto Hold = [&](UTurnBasedPlayerAction* Action)
    {
        Action->Activate(nullptr);
        Action->ShowHoverCursor(CursorClass, Tile);
        Action->SetTileCursorState(Tile, Grid_CursorState_Selected);
        TestEqual(TEXT("held: one cursor out"), ActiveCursors(World, CursorClass), 1);
    };
    const auto CheckClean = [&](const TCHAR* Exit)
    {
        TestEqual(*FString::Printf(TEXT("%s: cursor back"), Exit), ActiveCursors(World, CursorClass), 0);
        TestFalse(*FString::Printf(TEXT("%s: tile neutral"), Exit), Tile->GetCursorState().IsValid());
    };

    Hold(Place);
    Place->Complete();
    CheckClean(TEXT("Place complete"));
    Hold(Place);
    Place->Cancel();
    CheckClean(TEXT("Place cancel"));

    Hold(Swap);
    Swap->Cancel();
    CheckClean(TEXT("Swap cancel"));
    Hold(Swap);
    Swap->ForceDeactivate();
    CheckClean(TEXT("Swap force"));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
