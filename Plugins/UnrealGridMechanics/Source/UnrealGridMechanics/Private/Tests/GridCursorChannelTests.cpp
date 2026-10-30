// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "GridHoverTestTypes.h"
#include "GridMechanicsSettings.h"
#include "Hover/GridHoverComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GridCursorChannelTests
{
    constexpr EAutomationTestFlags TestFlags =
        EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

    // Sets the cursor trace channel for a scope
    struct FCursorChannel
    {
        UGridMechanicsSettings* Settings = GetMutableDefault<UGridMechanicsSettings>();
        TEnumAsByte<ECollisionChannel> Previous = Settings->CursorTraceChannel;
        explicit FCursorChannel(ECollisionChannel Channel) { Settings->CursorTraceChannel = Channel; }
        ~FCursorChannel() { Settings->CursorTraceChannel = Previous; }
    };
}

// Grid cursor channel plan, Stage 1: the plugin's Config/Engine.ini defines the
// GridCursor channel and the GridTile / GridPiece profiles; the setting is
// what the plugin reads
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridCursorChannelSettingsTest, "UnrealGridMechanics.Cursor.Settings",
    GridCursorChannelTests::TestFlags)

bool FGridCursorChannelSettingsTest::RunTest(const FString& Parameters)
{
    using namespace GridCursorChannelTests;
    UCollisionProfile* Profiles = UCollisionProfile::Get();
    TestEqual(TEXT("the plugin's ini: slot 18 is GridCursor"),
        Profiles->ReturnChannelNameFromContainerIndex(ECC_GameTraceChannel18), FName(TEXT("GridCursor")));

    FCollisionResponseTemplate Tile;
    FCollisionResponseTemplate Piece;
    TestTrue(TEXT("the GridTile profile exists and blocks GridCursor"),
        Profiles->GetProfileTemplate(TEXT("GridTile"), Tile) && Tile.ResponseToChannels.GetResponse(ECC_GameTraceChannel18) == ECR_Block);
    TestTrue(TEXT("the GridPiece profile exists and ignores GridCursor"),
        Profiles->GetProfileTemplate(TEXT("GridPiece"), Piece) && Piece.ResponseToChannels.GetResponse(ECC_GameTraceChannel18) == ECR_Ignore);

    {
        const FCursorChannel Grid(ECC_GameTraceChannel18);
        TestEqual(TEXT("the setting: read back"), UGridMechanicsSettings::GetCursorTraceChannel(), ECC_GameTraceChannel18);
        TestEqual(TEXT("the setting: named"), UGridMechanicsSettings::GetCursorTraceChannelName(), FString(TEXT("GridCursor")));
    }
    {
        const FCursorChannel Visibility(ECC_Visibility);
        TestEqual(TEXT("Visibility: read back"), UGridMechanicsSettings::GetCursorTraceChannel(), ECC_Visibility);
    }
    return true;
}

// Grid cursor channel plan, Stage 2: a spawned tile blocks the cursor channel
// and a piece ignores it with no Blueprint work; the hover check warns about
// an owner that blocks only Visibility
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridCursorChannelCollisionTest, "UnrealGridMechanics.Cursor.Collision",
    GridCursorChannelTests::TestFlags)

bool FGridCursorChannelCollisionTest::RunTest(const FString& Parameters)
{
    using namespace GridCursorChannelTests;
    const FCursorChannel Grid(ECC_GameTraceChannel18);

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GridCursorChannelTestWorld"));
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
    };

    // --- The base classes set their root primitive ---
    const AGridTestBoxTile* Tile = World->SpawnActor<AGridTestBoxTile>();
    const AGridTestBoxPiece* Piece = World->SpawnActor<AGridTestBoxPiece>();
    const UPrimitiveComponent* TileRoot = Tile ? Cast<UPrimitiveComponent>(Tile->GetRootComponent()) : nullptr;
    const UPrimitiveComponent* PieceRoot = Piece ? Cast<UPrimitiveComponent>(Piece->GetRootComponent()) : nullptr;
    if (!TestNotNull(TEXT("a tile"), TileRoot) || !TestNotNull(TEXT("a piece"), PieceRoot)) return false;
    TestEqual(TEXT("tile: blocks the cursor channel"), TileRoot->GetCollisionResponseToChannel(ECC_GameTraceChannel18), ECR_Block);
    TestEqual(TEXT("piece: ignores it"), PieceRoot->GetCollisionResponseToChannel(ECC_GameTraceChannel18), ECR_Ignore);

    // --- The hover check ---
    const auto MakeOwner = [World](ECollisionResponse Cursor, ECollisionResponse Visibility)
    {
        AActor* Owner = World->SpawnActor<AActor>();
        UBoxComponent* Box = NewObject<UBoxComponent>(Owner);
        Owner->SetRootComponent(Box);
        Box->RegisterComponent();
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Box->SetCollisionResponseToAllChannels(ECR_Ignore);
        Box->SetCollisionResponseToChannel(ECC_GameTraceChannel18, Cursor);
        Box->SetCollisionResponseToChannel(ECC_Visibility, Visibility);
        return NewObject<UGridHoverComponent>(Owner);
    };

    const UGridHoverComponent* Good = MakeOwner(ECR_Block, ECR_Ignore);
    TestTrue(TEXT("blocks the cursor channel: hoverable"), Good->DoesOwnerBlockCursorTrace() && Good->GetCursorTraceProblem().IsEmpty());

    const UGridHoverComponent* VisibilityOnly = MakeOwner(ECR_Ignore, ECR_Block);
    TestTrue(TEXT("blocks only Visibility: warned as such"), VisibilityOnly->GetCursorTraceProblem().Contains(TEXT("blocks Visibility but not")));

    const UGridHoverComponent* Nothing = MakeOwner(ECR_Ignore, ECR_Ignore);
    TestTrue(TEXT("blocks nothing: warned, naming the channel"), Nothing->GetCursorTraceProblem().Contains(TEXT("(GridCursor)")));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
