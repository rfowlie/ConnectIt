// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Action/ActionConfig.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Action/ConnectIt_BoardShiftAction.h"
#include "Action/ConnectIt_PlacePieceAction.h"
#include "Action/ConnectIt_SwapPieceAction.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ConnectItPlayerActionTests
{
    constexpr EAutomationTestFlags TestFlags =
        EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

    FPermanentActionConfig MakeConfig(TSubclassOf<UTurnBasedPlayerAction> ActionClass,
        int32 MatchUses, int32 TurnUses, int32 CooldownTurns = 0)
    {
        FPermanentActionConfig Config;
        Config.ActionClass = ActionClass;
        Config.MatchUses = MatchUses;
        Config.TurnUses = TurnUses;
        Config.CooldownTurns = CooldownTurns;
        return Config;
    }

    // A throwaway game world, so a PlayerState can be spawned with authority
    struct FTestWorld
    {
        UWorld* World = nullptr;

        FTestWorld()
        {
            World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("ConnectItPlayerActionTestWorld"));
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        }

        ~FTestWorld()
        {
            GEngine->DestroyWorldContext(World);
            World->DestroyWorld(false);
        }

        ATurnBasedPlayerState* SpawnPlayerState() const
        {
            return World->SpawnActor<ATurnBasedPlayerState>();
        }
    };
}

// Permanent actions: match uses (finite or unlimited), turn uses, cooldown,
// seeded from a game-built list
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItPermanentActionUsesTest,
    "ConnectIt.PlayerActions.PermanentUses", ConnectItPlayerActionTests::TestFlags)

bool FConnectItPermanentActionUsesTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItPlayerActionTests;
    const FTestWorld TestWorld;
    ATurnBasedPlayerState* PS = TestWorld.SpawnPlayerState();
    if (!TestNotNull(TEXT("player state spawned"), PS)) return false;

    const TSubclassOf<UTurnBasedPlayerAction> Place = UConnectIt_PlacePieceAction::StaticClass();
    const TSubclassOf<UTurnBasedPlayerAction> Swap = UConnectIt_SwapPieceAction::StaticClass();
    const TSubclassOf<UTurnBasedPlayerAction> Shift = UConnectIt_BoardShiftAction::StaticClass();

    TestFalse(TEXT("nothing seeded: no action config"), PS->HasActionConfig());

    // A game-built list: Place unlimited (1 a turn), Swap 2 in the match (2 a
    // turn), Shift unlimited with a 2-turn cooldown. A second Place entry is
    // ignored; a turn limit below 1 becomes 1.
    PS->InitialiseActionState(nullptr, {
        MakeConfig(Place, -1, 1),
        MakeConfig(Swap, 2, 2),
        MakeConfig(Shift, -1, 0, 2),
        MakeConfig(Place, 5, 5),
    });
    TestTrue(TEXT("seeded: action config"), PS->HasActionConfig());
    TestEqual(TEXT("three actions (the duplicate ignored)"), PS->GetPermanentActionState().Num(), 3);
    TestEqual(TEXT("Place: unlimited match uses"), PS->GetMatchUsesRemaining(Place), -1);
    TestEqual(TEXT("Place: the first entry's turn limit"), PS->GetActionMaxUsesPerTurn(Place), 1);
    TestEqual(TEXT("Shift: turn limit raised to 1"), PS->GetActionMaxUsesPerTurn(Shift), 1);

    // Turn uses cap a turn; unlimited match uses never run out
    TestTrue(TEXT("Place: first use"), PS->ConsumeActionUse(Place));
    TestFalse(TEXT("Place: turn limit reached"), PS->CanUseAction(Place));
    TestFalse(TEXT("Place: a second use this turn is refused"), PS->ConsumeActionUse(Place));
    PS->ResetActionTurnCounters();
    TestTrue(TEXT("Place: usable next turn"), PS->ConsumeActionUse(Place));
    TestEqual(TEXT("Place: still unlimited"), PS->GetMatchUsesRemaining(Place), -1);

    // A finite match budget runs out
    TestTrue(TEXT("Swap: first use"), PS->ConsumeActionUse(Swap));
    TestTrue(TEXT("Swap: second use, same turn"), PS->ConsumeActionUse(Swap));
    TestEqual(TEXT("Swap: none left"), PS->GetMatchUsesRemaining(Swap), 0);
    PS->ResetActionTurnCounters();
    TestFalse(TEXT("Swap: none left next turn either"), PS->CanUseAction(Swap));

    // Cooldown
    TestTrue(TEXT("Shift: used"), PS->ConsumeActionUse(Shift));
    PS->ResetActionTurnCounters();
    TestFalse(TEXT("Shift: cooling down (2)"), PS->CanUseAction(Shift));
    PS->TickActionCooldowns();
    TestFalse(TEXT("Shift: cooling down (1)"), PS->CanUseAction(Shift));
    PS->TickActionCooldowns();
    TestTrue(TEXT("Shift: ready again"), PS->CanUseAction(Shift));

    // An action the player wasn't given
    TestFalse(TEXT("an action not in the list can't be used"),
        PS->CanUseAction(UConnectIt_PlacePieceAction::StaticClass()->GetSuperClass()));
    return true;
}

// The loadout's own list seeds by default, and its validation
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItPermanentActionLoadoutTest,
    "ConnectIt.PlayerActions.Loadout", ConnectItPlayerActionTests::TestFlags)

bool FConnectItPermanentActionLoadoutTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItPlayerActionTests;
    const FTestWorld TestWorld;
    ATurnBasedPlayerState* PS = TestWorld.SpawnPlayerState();
    if (!TestNotNull(TEXT("player state spawned"), PS)) return false;

    UActionLoadoutDataAsset* Loadout = NewObject<UActionLoadoutDataAsset>();
    Loadout->PermanentActions.Add(MakeConfig(UConnectIt_PlacePieceAction::StaticClass(), -1, 1));
    Loadout->PermanentActions.Add(MakeConfig(UConnectIt_SwapPieceAction::StaticClass(), 3, 1));

    PS->InitialiseActionState(Loadout);
    TestEqual(TEXT("seeded from the loadout"), PS->GetPermanentActionState().Num(), 2);
    TestEqual(TEXT("Swap: the loadout's match uses"),
        PS->GetMatchUsesRemaining(UConnectIt_SwapPieceAction::StaticClass()), 3);
    TestTrue(TEXT("the loadout is set"), PS->GetLoadout() == Loadout);

    // A game-built list replaces it, keeping the loadout
    PS->InitialiseActionState(Loadout, { MakeConfig(UConnectIt_SwapPieceAction::StaticClass(), 1, 1) });
    TestEqual(TEXT("the game's list replaces the loadout's"), PS->GetPermanentActionState().Num(), 1);
    TestEqual(TEXT("Place not given"), PS->GetMatchUsesRemaining(UConnectIt_PlacePieceAction::StaticClass()), 0);
    TestTrue(TEXT("the loadout still set"), PS->GetLoadout() == Loadout);

#if WITH_EDITOR
    // Validation: a duplicate is an error; 0 match uses a warning
    UActionLoadoutDataAsset* Broken = NewObject<UActionLoadoutDataAsset>();
    Broken->PermanentActions.Add(MakeConfig(UConnectIt_PlacePieceAction::StaticClass(), -1, 1));
    Broken->PermanentActions.Add(MakeConfig(UConnectIt_PlacePieceAction::StaticClass(), 0, 1));
    FDataValidationContext Validation;
    TestTrue(TEXT("a duplicate: invalid"), Broken->IsDataValid(Validation) == EDataValidationResult::Invalid);

    UActionLoadoutDataAsset* Unusable = NewObject<UActionLoadoutDataAsset>();
    Unusable->PermanentActions.Add(MakeConfig(UConnectIt_SwapPieceAction::StaticClass(), 0, 1));
    FDataValidationContext Warned;
    Unusable->IsDataValid(Warned);
    bool bWarnedAboutZero = false;
    for (const FDataValidationContext::FIssue& Issue : Warned.GetIssues())
    {
        bWarnedAboutZero |= Issue.Message.ToString().Contains(TEXT("MatchUses 0"));
    }
    TestTrue(TEXT("0 match uses: warned"), bWarnedAboutZero);
#endif
    return true;
}

#endif
