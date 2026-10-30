// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "GridHoverTestTypes.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridCursorStateTags.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS

// A tile's cursor state: a change raises one event with the right previous /
// current; the same state again raises nothing; clear -> neutral
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTileCursorStateTest, "UnrealGridMechanics.Tile.CursorState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGridTileCursorStateTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GridTileCursorStateTestWorld"));
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
    };

    AGridTileBase* Tile = World->SpawnActor<AGridTileBase>();
    if (!TestNotNull(TEXT("a tile"), Tile)) return false;
    UGridCursorStateTestListener* Listener = NewObject<UGridCursorStateTestListener>();
    Listener->Listen(Tile);

    TestFalse(TEXT("starts neutral"), Tile->GetCursorState().IsValid());

    Tile->SetCursorState(Grid_CursorState_Hovered);
    TestEqual(TEXT("hovered: one change"), Listener->Changes, 1);
    TestTrue(TEXT("hovered: neutral -> hovered"),
        !Listener->LastPrevious.IsValid() && Listener->LastCurrent == Grid_CursorState_Hovered);
    TestTrue(TEXT("hovered: stored"), Tile->GetCursorState() == Grid_CursorState_Hovered);

    Tile->SetCursorState(Grid_CursorState_Hovered);
    TestEqual(TEXT("the same state: nothing"), Listener->Changes, 1);

    Tile->SetCursorState(Grid_CursorState_Selected);
    TestEqual(TEXT("selected: a second change"), Listener->Changes, 2);
    TestTrue(TEXT("selected: hovered -> selected"),
        Listener->LastPrevious == Grid_CursorState_Hovered && Listener->LastCurrent == Grid_CursorState_Selected);

    Tile->ClearCursorState();
    TestEqual(TEXT("clear: a third change"), Listener->Changes, 3);
    TestFalse(TEXT("clear: neutral"), Tile->GetCursorState().IsValid());
    Tile->ClearCursorState();
    TestEqual(TEXT("clear again: nothing"), Listener->Changes, 3);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
