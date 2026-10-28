// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "GridHoverTestTypes.h"
#include "Hover/GridHoverComponent.h"
#include "Tile/GridTileBase.h"
#include "Subsystem/GridHoverSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GridHoverTests
{
    constexpr EAutomationTestFlags TestFlags =
        EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

// The hover component's relay rules, with no world: what it broadcasts when
// enabled, disabled and vetoed (hover end and click included)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridHoverComponentTest,
    "UnrealGridMechanics.Hover.Component", GridHoverTests::TestFlags)

bool FGridHoverComponentTest::RunTest(const FString& Parameters)
{
    // --- Enabled: begin, end and click relay; a repeat begin / stray end doesn't ---
    {
        UGridHoverComponent* Hover = NewObject<UGridHoverComponent>();
        UGridHoverTestListener* Listener = NewObject<UGridHoverTestListener>();
        Listener->Listen(Hover);

        Hover->NotifyHoverBegin();
        Hover->NotifyHoverBegin();
        TestTrue(TEXT("enabled: hovered"), Hover->IsHovered());
        Hover->NotifyClicked();
        Hover->NotifyHoverEnd();
        Hover->NotifyHoverEnd();
        TestFalse(TEXT("enabled: not hovered after the end"), Hover->IsHovered());
        TestEqual(TEXT("enabled: one begin"), Listener->Begins, 1);
        TestEqual(TEXT("enabled: one end"), Listener->Ends, 1);
        TestEqual(TEXT("enabled: one click"), Listener->Clicks, 1);
    }

    // --- Disabled: nothing relays; disabling during a hover ends it ---
    {
        UGridHoverComponent* Hover = NewObject<UGridHoverComponent>();
        UGridHoverTestListener* Listener = NewObject<UGridHoverTestListener>();
        Listener->Listen(Hover);

        Hover->NotifyHoverBegin();
        Hover->SetHoverEnabled(false);
        TestFalse(TEXT("disabled: off"), Hover->IsHoverEnabled());
        TestEqual(TEXT("disabled during a hover: the hover ends"), Listener->Ends, 1);

        Hover->NotifyHoverBegin();
        Hover->NotifyClicked();
        Hover->NotifyHoverEnd();
        TestEqual(TEXT("disabled: no begin"), Listener->Begins, 1);
        TestEqual(TEXT("disabled: no click"), Listener->Clicks, 0);
        TestEqual(TEXT("disabled: no further end"), Listener->Ends, 1);

        Hover->SetHoverEnabled(true);
        Hover->NotifyHoverBegin();
        TestEqual(TEXT("enabled again: relays"), Listener->Begins, 2);
    }

    // --- CanHover vetoes hover and click ---
    {
        UGridHoverTestVetoComponent* Hover = NewObject<UGridHoverTestVetoComponent>();
        UGridHoverTestListener* Listener = NewObject<UGridHoverTestListener>();
        Listener->Listen(Hover);

        Hover->NotifyHoverBegin();
        Hover->NotifyClicked();
        TestFalse(TEXT("vetoed: not hovered"), Hover->IsHovered());
        TestEqual(TEXT("vetoed: nothing relayed"), Listener->Begins + Listener->Clicks, 0);

        Hover->bAllow = true;
        Hover->NotifyHoverBegin();
        Hover->NotifyClicked();
        TestEqual(TEXT("allowed: begin and click relayed"), Listener->Begins + Listener->Clicks, 2);
    }

    // --- Every tile class has one ---
    {
        const AGridTileBase* Tile = GetDefault<AGridTileBase>();
        TestNotNull(TEXT("a tile has a hover component"), Tile->HoverComponent.Get());
    }
    return true;
}

// UGridHoverSubsystem and the tiles' hover components, in a throwaway world:
// registering binds and enables a tile's component, unregistering ends its
// hover and unbinds it; begin, end and click reach the relays
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridHoverSubsystemTest,
    "UnrealGridMechanics.Hover.Subsystem", GridHoverTests::TestFlags)

bool FGridHoverSubsystemTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GridHoverTestWorld"));
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
    };

    UGridHoverSubsystem* Subsystem = World->GetSubsystem<UGridHoverSubsystem>();
    AGridTileBase* Tile = World->SpawnActor<AGridTileBase>();
    if (!TestNotNull(TEXT("subsystem"), Subsystem) || !TestNotNull(TEXT("tile spawned"), Tile)) return false;
    UGridHoverComponent* Hover = Tile->HoverComponent;
    UGridHoverTestTileListener* Listener = NewObject<UGridHoverTestTileListener>();
    Listener->Listen(Subsystem);

    // --- Not registered: nothing reaches the relays ---
    Hover->NotifyHoverBegin();
    Hover->NotifyHoverEnd();
    TestEqual(TEXT("unregistered: nothing relayed"), Listener->Begins + Listener->Ends, 0);

    // --- Registered: begin, click and end relay, naming the tile ---
    Hover->SetHoverEnabled(false);
    Subsystem->RegisterTile(Tile);
    TestTrue(TEXT("registered"), Subsystem->IsTileRegistered(Tile));
    TestTrue(TEXT("registering enables the component"), Hover->IsHoverEnabled());
    Hover->NotifyHoverBegin();
    TestEqual(TEXT("registered: hover begin relayed"), Listener->Begins, 1);
    TestTrue(TEXT("registered: for this tile"), Listener->LastTile == Tile);
    Hover->NotifyClicked();
    TestEqual(TEXT("registered: click relayed"), Listener->Clicks, 1);
    Hover->NotifyHoverEnd();
    TestEqual(TEXT("registered: hover end relayed"), Listener->Ends, 1);

    // --- Unregistering during a hover ends it, then unbinds ---
    Hover->NotifyHoverBegin();
    Subsystem->UnregisterTile(Tile);
    TestFalse(TEXT("unregistered"), Subsystem->IsTileRegistered(Tile));
    TestFalse(TEXT("unregistering disables the component"), Hover->IsHoverEnabled());
    TestEqual(TEXT("unregistered while hovered: the end is relayed"), Listener->Ends, 2);

    Hover->SetHoverEnabled(true);
    Hover->NotifyHoverBegin();
    Hover->NotifyClicked();
    TestEqual(TEXT("unbound: no further begin"), Listener->Begins, 2);
    TestEqual(TEXT("unbound: no further click"), Listener->Clicks, 1);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
