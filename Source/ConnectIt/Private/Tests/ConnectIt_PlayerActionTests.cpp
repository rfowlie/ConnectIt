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
#include "Action/ItemActionInventoryDataAsset.h"
#include "Action/TurnEndRequirement.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "TurnBasedMechanicsSettings.h"

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

    FItemActionEntry MakeItems(TSubclassOf<UTurnBasedPlayerAction> ActionClass, int32 Count)
    {
        FItemActionEntry Entry;
        Entry.ActionClass = ActionClass;
        Entry.Count = Count;
        return Entry;
    }

    // Makes Caps the project's item inventory asset until destroyed (the
    // settings' own asset is put back)
    struct FScopedItemCaps
    {
        TSoftObjectPtr<UItemActionInventoryDataAsset> Previous;

        explicit FScopedItemCaps(UItemActionInventoryDataAsset* Caps)
        {
            UTurnBasedMechanicsSettings* Settings = GetMutableDefault<UTurnBasedMechanicsSettings>();
            Previous = Settings->ItemInventory;
            Settings->ItemInventory = Caps;
        }

        ~FScopedItemCaps()
        {
            GetMutableDefault<UTurnBasedMechanicsSettings>()->ItemInventory = Previous;
        }
    };

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

// Item actions: the inventory (caps from the game's item inventory asset,
// grants), the item budget, and item uses never touching permanent counters
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItItemActionsTest,
    "ConnectIt.PlayerActions.Items", ConnectItPlayerActionTests::TestFlags)

bool FConnectItItemActionsTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItPlayerActionTests;
    const FTestWorld TestWorld;
    ATurnBasedPlayerState* PS = TestWorld.SpawnPlayerState();
    if (!TestNotNull(TEXT("player state spawned"), PS)) return false;

    const TSubclassOf<UTurnBasedPlayerAction> Place = UConnectIt_PlacePieceAction::StaticClass();
    const TSubclassOf<UTurnBasedPlayerAction> Shift = UConnectIt_BoardShiftAction::StaticClass();
    const TSubclassOf<UTurnBasedPlayerAction> Swap = UConnectIt_SwapPieceAction::StaticClass();
    const TSubclassOf<UTurnBasedPlayerAction> Unlisted = UConnectIt_PlacePieceAction::StaticClass()->GetSuperClass();

    // The game's caps: Place 3, Shift 2, Swap 1. Anything else can't be held.
    UItemActionInventoryDataAsset* Caps = NewObject<UItemActionInventoryDataAsset>();
    Caps->MaxHeld.Add(Place, 3);
    Caps->MaxHeld.Add(Shift, 2);
    Caps->MaxHeld.Add(Swap, 1);
    const FScopedItemCaps UseCaps(Caps);

    // Permanent: Place only. Items: 2 Place, 5 Shift (capped at 2), a Swap
    // holding none (dropped), and an unlisted action (dropped: can't be held).
    PS->InitialiseActionState(nullptr, { MakeConfig(Place, -1, 1) });
    PS->InitialiseInventory({ MakeItems(Place, 2), MakeItems(Shift, 5), MakeItems(Swap, 0), MakeItems(Unlisted, 1) });
    TestEqual(TEXT("seeding clamps to the cap"), PS->GetItemCount(Shift), 2);
    TestEqual(TEXT("only held items are kept (Swap and the unlisted action dropped)"), PS->GetInventory().Num(), 2);

    // No budget (the default): no item can be used
    TestFalse(TEXT("no budget: no items"), PS->CanUseItem(Place));
    TestFalse(TEXT("no budget: consume refused"), PS->ConsumeItemUse(Place));

    // A budget of 3 a match, 1 a turn
    FItemBudget Budget; Budget.MatchUses = 3; Budget.TurnUses = 1;
    PS->SetItemBudget(Budget);

    // An item use spends the item and the budget, not the permanent uses
    TestTrue(TEXT("Place item used"), PS->ConsumeItemUse(Place));
    TestEqual(TEXT("one Place item left"), PS->GetItemCount(Place), 1);
    TestEqual(TEXT("Place's permanent uses untouched"), PS->GetActionUsesThisTurn(Place), 0);
    TestTrue(TEXT("Place still usable as a permanent action"), PS->CanUseAction(Place));

    // The budget's turn limit covers every item action
    TestFalse(TEXT("turn budget spent: Shift refused"), PS->CanUseItem(Shift));
    PS->ResetActionTurnCounters();

    // An item action with no permanent uses can still be used as an item
    TestFalse(TEXT("Shift: no permanent uses"), PS->CanUseAction(Shift));
    TestTrue(TEXT("Shift item used"), PS->ConsumeItemUse(Shift));
    PS->ResetActionTurnCounters();
    TestTrue(TEXT("third item use of the match"), PS->ConsumeItemUse(Shift));
    PS->ResetActionTurnCounters();
    TestEqual(TEXT("three item uses this match"), PS->GetItemUsesThisMatch(), 3);
    TestFalse(TEXT("match budget spent"), PS->CanUseItem(Place));

    // The last Shift spent: it leaves the inventory
    TestEqual(TEXT("no Shift items left"), PS->GetItemCount(Shift), 0);
    TestEqual(TEXT("Shift left the inventory"), PS->GetInventory().Num(), 1);

    // Grants: clamped to the cap, never touch the budget; an action not held
    // yet can be granted; an unlisted one can't
    TestEqual(TEXT("grant 5 Place: 2 fit (1 held, cap 3)"), PS->GrantItems(Place, 5), 2);
    TestEqual(TEXT("Place at its cap"), PS->GetItemCount(Place), 3);
    TestEqual(TEXT("a Swap not held: granted up to its cap"), PS->GrantItems(Swap, 4), 1);
    TestEqual(TEXT("Swap now held"), PS->GetItemCount(Swap), 1);
    TestEqual(TEXT("an unlisted action: nothing"), PS->GrantItems(Unlisted, 1), 0);
    TestEqual(TEXT("grants didn't touch the budget"), PS->GetItemUsesThisMatch(), 3);
    return true;
}

// The Item Used turn-end leaf, alone and grouped with an Action Used leaf;
// an item use never satisfies an Action Used leaf
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItItemUsedTurnEndTest,
    "ConnectIt.PlayerActions.ItemUsedTurnEnd", ConnectItPlayerActionTests::TestFlags)

bool FConnectItItemUsedTurnEndTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItPlayerActionTests;
    const FTestWorld TestWorld;
    ATurnBasedPlayerState* PS = TestWorld.SpawnPlayerState();
    if (!TestNotNull(TEXT("player state spawned"), PS)) return false;

    const TSubclassOf<UTurnBasedPlayerAction> Place = UConnectIt_PlacePieceAction::StaticClass();
    UItemActionInventoryDataAsset* Caps = NewObject<UItemActionInventoryDataAsset>();
    Caps->MaxHeld.Add(Place, 3);
    const FScopedItemCaps UseCaps(Caps);

    PS->InitialiseActionState(nullptr, { MakeConfig(Place, -1, 1) });
    PS->InitialiseInventory({ MakeItems(Place, 3) });
    FItemBudget Budget; Budget.MatchUses = -1; Budget.TurnUses = 2;
    PS->SetItemBudget(Budget);

    // Alone: "an item was used"
    UActionLoadoutDataAsset* ItemOnly = NewObject<UActionLoadoutDataAsset>();
    ItemOnly->TurnEndRequirements = NewObject<UTurnEndRequirementItemUsed>(ItemOnly);
    TestFalse(TEXT("item leaf: not yet"), PS->AreTurnEndRequirementsMet(ItemOnly));
    PS->ConsumeItemUse(Place);
    TestTrue(TEXT("item leaf: an item used"), PS->AreTurnEndRequirementsMet(ItemOnly));
    PS->ResetActionTurnCounters();
    TestFalse(TEXT("item leaf: a new turn"), PS->AreTurnEndRequirementsMet(ItemOnly));

    // An item Place doesn't count as Place for an Action Used leaf
    UActionLoadoutDataAsset* PlaceOnly = NewObject<UActionLoadoutDataAsset>();
    UTurnEndRequirementLeaf* PlaceLeaf = NewObject<UTurnEndRequirementLeaf>(PlaceOnly);
    PlaceLeaf->ActionClass = Place;
    PlaceOnly->TurnEndRequirements = PlaceLeaf;
    PS->ConsumeItemUse(Place);
    TestFalse(TEXT("an item Place doesn't satisfy Place"), PS->AreTurnEndRequirementsMet(PlaceOnly));
    PS->ResetActionTurnCounters();

    // Grouped: Place, all with an item
    UActionLoadoutDataAsset* Both = NewObject<UActionLoadoutDataAsset>();
    UTurnEndRequirementGroup* All = NewObject<UTurnEndRequirementGroup>(Both);
    All->Mode = ETurnEndCombineMode::All;
    UTurnEndRequirementLeaf* GroupPlace = NewObject<UTurnEndRequirementLeaf>(All);
    GroupPlace->ActionClass = Place;
    All->Children.Add(GroupPlace);
    All->Children.Add(NewObject<UTurnEndRequirementItemUsed>(All));
    Both->TurnEndRequirements = All;

    PS->ConsumeActionUse(Place);
    TestFalse(TEXT("all: Place alone isn't enough"), PS->AreTurnEndRequirementsMet(Both));
    PS->ConsumeItemUse(Place);
    TestTrue(TEXT("all: Place and an item"), PS->AreTurnEndRequirementsMet(Both));
    return true;
}

#endif
