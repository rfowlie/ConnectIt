// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/DataValidation.h"
#include "NativeGameplayTags.h"
#include "Adventure/ConnectIt_AdventureProgress.h"
#include "Adventure/ConnectIt_MapNodeDataAsset.h"
#include "Adventure/ConnectIt_ActionShardTableDataAsset.h"
#include "Action/ItemActionInventoryDataAsset.h"
#include "Action/ConnectIt_PlacePieceAction.h"
#include "Action/ConnectIt_SwapPieceAction.h"
#include "Action/ConnectIt_BoardShiftAction.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "TurnBasedMechanicsSettings.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ConnectItAdventureTests
{
    constexpr EAutomationTestFlags TestFlags =
        EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

    UE_DEFINE_GAMEPLAY_TAG_STATIC(TestAction, "ConnectIt.PlayerAction.Test");
    UE_DEFINE_GAMEPLAY_TAG_STATIC(TestOtherAction, "ConnectIt.PlayerAction.TestOther");
    UE_DEFINE_GAMEPLAY_TAG_STATIC(TestSeal, "ConnectIt.Adventure.Seal.Test");
    UE_DEFINE_GAMEPLAY_TAG_STATIC(TestOtherSeal, "ConnectIt.Adventure.Seal.TestOther");

    // A table where TestAction's uses cost 3, 5 and 8 shards: uses at 3, 8 and
    // 16 shards in all
    UConnectIt_ActionShardTableDataAsset* MakeTable()
    {
        UConnectIt_ActionShardTableDataAsset* Table = NewObject<UConnectIt_ActionShardTableDataAsset>();
        FConnectItActionShardCosts Costs;
        Costs.ShardsPerUse = { 3, 5, 8 };
        Table->Actions.Add(TestAction, Costs);
        return Table;
    }

    // A level node needing TestAction, TestSeal and one general seal
    UConnectIt_MapNodeDataAsset* MakeGatedNode()
    {
        UConnectIt_MapNodeDataAsset* Node = NewObject<UConnectIt_MapNodeDataAsset>();
        Node->Level = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Test/TestLevel.TestLevel")));
        Node->Gate.RequiredActions.AddTag(TestAction);
        Node->Gate.RequiredSeals.AddTag(TestSeal);
        Node->Gate.GeneralSeals = 1;
        return Node;
    }
}

// Shards become permanent uses through a action's table: each use costs its
// entry's shards on top of the earlier uses'
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAdventureShardTableTest,
    "ConnectIt.Adventure.ShardTable", ConnectItAdventureTests::TestFlags)

bool FConnectItAdventureShardTableTest::RunTest(const FString& Parameters)
{
    const UConnectIt_ActionShardTableDataAsset* Table = ConnectItAdventureTests::MakeTable();

    TestEqual(TEXT("0 shards: no use"), Table->GetPermanentUses(ConnectItAdventureTests::TestAction, 0), 0);
    TestEqual(TEXT("2 shards: no use"), Table->GetPermanentUses(ConnectItAdventureTests::TestAction, 2), 0);
    TestEqual(TEXT("3 shards: 1 use"), Table->GetPermanentUses(ConnectItAdventureTests::TestAction, 3), 1);
    TestEqual(TEXT("7 shards: still 1 use"), Table->GetPermanentUses(ConnectItAdventureTests::TestAction, 7), 1);
    TestEqual(TEXT("8 shards (3 + 5): 2 uses"), Table->GetPermanentUses(ConnectItAdventureTests::TestAction, 8), 2);
    TestEqual(TEXT("15 shards: still 2 uses"), Table->GetPermanentUses(ConnectItAdventureTests::TestAction, 15), 2);
    TestEqual(TEXT("16 shards (3 + 5 + 8): 3 uses"), Table->GetPermanentUses(ConnectItAdventureTests::TestAction, 16), 3);
    TestEqual(TEXT("past the table: still 3"), Table->GetPermanentUses(ConnectItAdventureTests::TestAction, 100), 3);
    TestEqual(TEXT("a action with no entry: none"), Table->GetPermanentUses(ConnectItAdventureTests::TestOtherAction, 100), 0);

    // Through a progress value (shards are what's saved; uses are derived)
    FConnectItAdventureProgress Progress;
    Progress.Shards.Add(ConnectItAdventureTests::TestAction, 5);
    TestEqual(TEXT("progress: 5 shards = 1 use"), Progress.GetPermanentUses(ConnectItAdventureTests::TestAction, Table), 1);
    TestEqual(TEXT("progress: no table = none"), Progress.GetPermanentUses(ConnectItAdventureTests::TestAction, nullptr), 0);
    Progress.Shards.Add(ConnectItAdventureTests::TestAction, 8);
    TestEqual(TEXT("progress: more shards, more uses"), Progress.GetPermanentUses(ConnectItAdventureTests::TestAction, Table), 2);

    // Falling costs are only a warning, and still count: 5 then 3 = uses at 5 and 8
    // (A real action: an entry names its action class, whose tag is its key)
    const TSubclassOf<UTurnBasedPlayerAction> Swap = UConnectIt_SwapPieceAction::StaticClass();
    const FGameplayTag SwapTag = UTurnBasedPlayerAction::GetTagForClass(Swap);
    UConnectIt_ActionShardTableDataAsset* Falling = NewObject<UConnectIt_ActionShardTableDataAsset>();
    FConnectItActionShardCosts Costs;
    Costs.ActionClass = Swap;
    Costs.ShardsPerUse = { 5, 3 };
    Falling->Actions.Add(SwapTag, Costs);
#if WITH_EDITOR
    FDataValidationContext Validation;
    TestTrue(TEXT("falling: still valid"), Falling->IsDataValid(Validation) != EDataValidationResult::Invalid);
    TestEqual(TEXT("falling: one warning"), static_cast<int32>(Validation.GetNumWarnings()), 1);
#endif
    TestEqual(TEXT("falling: 7 shards = 1 use"), Falling->GetPermanentUses(SwapTag, 7), 1);
    TestEqual(TEXT("falling: 8 shards = 2 uses"), Falling->GetPermanentUses(SwapTag, 8), 2);
    return true;
}

// What a gated node still needs, against a progress value
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAdventureNodeRequirementsTest,
    "ConnectIt.Adventure.NodeRequirements", ConnectItAdventureTests::TestFlags)

bool FConnectItAdventureNodeRequirementsTest::RunTest(const FString& Parameters)
{
    const UConnectIt_ActionShardTableDataAsset* Table = ConnectItAdventureTests::MakeTable();
    const UConnectIt_MapNodeDataAsset* Node = ConnectItAdventureTests::MakeGatedNode();

    FConnectItAdventureProgress Progress;
    FConnectItNodeRequirementCheck Check = Node->CheckRequirements(Progress, Table);
    TestFalse(TEXT("nothing yet: not met"), Check.IsMet());
    TestTrue(TEXT("nothing yet: missing the action"), Check.MissingActions.HasTagExact(ConnectItAdventureTests::TestAction));
    TestTrue(TEXT("nothing yet: missing the seal"), Check.MissingSeals.HasTagExact(ConnectItAdventureTests::TestSeal));
    TestEqual(TEXT("nothing yet: one general seal short"), Check.MissingGeneralSeals, 1);

    // Item uses never count towards a required action
    Progress.ItemUses.Add(ConnectItAdventureTests::TestAction, 5);
    Progress.Shards.Add(ConnectItAdventureTests::TestAction, 2);
    Check = Node->CheckRequirements(Progress, Table);
    TestTrue(TEXT("item uses and too few shards: action still missing"), Check.MissingActions.HasTagExact(ConnectItAdventureTests::TestAction));

    // Enough shards for a permanent use (the 1st costs 3)
    Progress.Shards.Add(ConnectItAdventureTests::TestAction, 3);
    Check = Node->CheckRequirements(Progress, Table);
    TestTrue(TEXT("a permanent use: action met"), Check.MissingActions.IsEmpty());
    TestTrue(TEXT("seals still missing"), Check.IsMissingSeals());

    // The wrong specific seal doesn't open it
    Progress.SpecificSeals.AddTag(ConnectItAdventureTests::TestOtherSeal);
    Progress.GeneralSeals = 1;
    Check = Node->CheckRequirements(Progress, Table);
    TestTrue(TEXT("another seal: still missing this one"), Check.MissingSeals.HasTagExact(ConnectItAdventureTests::TestSeal));
    TestEqual(TEXT("one general seal held: none short"), Check.MissingGeneralSeals, 0);

    Progress.SpecificSeals.AddTag(ConnectItAdventureTests::TestSeal);
    TestTrue(TEXT("action and seals: met"), Node->CheckRequirements(Progress, Table).IsMet());

    // A broken seal needs no seals any more -- but required actions still count
    FConnectItAdventureProgress Broken;
    Broken.BrokenSeals.Add(Node->NodeId);
    Check = Node->CheckRequirements(Broken, Table);
    TestFalse(TEXT("broken seal: no seals needed"), Check.IsMissingSeals());
    TestTrue(TEXT("broken seal: action still needed"), Check.MissingActions.HasTagExact(ConnectItAdventureTests::TestAction));
    return true;
}

// A node's id, and validation of what it plays and unlocks
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAdventureNodeDataTest,
    "ConnectIt.Adventure.NodeData", ConnectItAdventureTests::TestFlags)

bool FConnectItAdventureNodeDataTest::RunTest(const FString& Parameters)
{
    UConnectIt_MapNodeDataAsset* Node = ConnectItAdventureTests::MakeGatedNode();
    TestTrue(TEXT("a new node has an id"), Node->NodeId.IsValid());

    const UConnectIt_MapNodeDataAsset* Copy = DuplicateObject(Node, GetTransientPackage());
    TestTrue(TEXT("a copy has its own id"), Copy->NodeId.IsValid() && Copy->NodeId != Node->NodeId);

#if WITH_EDITOR
    FDataValidationContext Valid;
    TestTrue(TEXT("a level node: valid"), Node->IsDataValid(Valid) != EDataValidationResult::Invalid);

    // Neither a level nor a sub-map, and unlocking itself
    UConnectIt_MapNodeDataAsset* Broken = NewObject<UConnectIt_MapNodeDataAsset>();
    Broken->Unlocks.Add(Broken);
    FDataValidationContext Validation;
    TestTrue(TEXT("plays nothing, unlocks itself: invalid"),
        Broken->IsDataValid(Validation) == EDataValidationResult::Invalid);
    TestEqual(TEXT("both reported"), static_cast<int32>(Validation.GetNumErrors()), 2);

    // A closed node: no items by default; an item budget is warned about
    UConnectIt_MapNodeDataAsset* Closed = ConnectItAdventureTests::MakeGatedNode();
    Closed->ActionDesign = EConnectItActionDesign::Closed;
    TestEqual(TEXT("the default item budget allows none"), Closed->ItemBudget.TurnUses, 0);
    FDataValidationContext NoItems;
    Closed->IsDataValid(NoItems);
    Closed->ItemBudget.MatchUses = 2;
    Closed->ItemBudget.TurnUses = 1;
    FDataValidationContext WithItems;
    Closed->IsDataValid(WithItems);
    TestEqual(TEXT("closed with an item budget: one more warning"),
        static_cast<int32>(WithItems.GetNumWarnings()), static_cast<int32>(NoItems.GetNumWarnings()) + 1);

    // Both a level and a sub-map
    Node->SubMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Test/TestHub.TestHub")));
    FDataValidationContext Both;
    TestTrue(TEXT("a level and a sub-map: invalid"), Node->IsDataValid(Both) == EDataValidationResult::Invalid);
#endif
    return true;
}

// Adventure hub plan, Stage 2: a node's state through gate, seal break and win
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAdventureNodeStateTest,
    "ConnectIt.Adventure.NodeState", ConnectItAdventureTests::TestFlags)

bool FConnectItAdventureNodeStateTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItAdventureTests;
    const UConnectIt_ActionShardTableDataAsset* Table = MakeTable();
    const UConnectIt_MapNodeDataAsset* Node = MakeGatedNode();
    FConnectItAdventureProgress Progress;
    FConnectItNodeRequirementCheck Check;

    TestEqual(TEXT("not revealed: hidden"), Progress.GetNodeState(*Node, false, Table, Check), EConnectItNodeState::Hidden);
    TestEqual(TEXT("revealed, needs everything: gated"), Progress.GetNodeState(*Node, true, Table, Check), EConnectItNodeState::Gated);

    // --- The seal: not affordable, then broken (the seals are spent) ---
    FString WhyNot;
    TestFalse(TEXT("seal: nothing held, can't break"), Progress.CanBreakSeal(*Node, &WhyNot));
    TestFalse(TEXT("seal: and says why"), WhyNot.IsEmpty());
    TestFalse(TEXT("seal: breaking fails without change"), Progress.BreakSeal(*Node) || Progress.IsSealBroken(Node->NodeId));

    Progress.SpecificSeals.AddTag(TestSeal);
    Progress.SpecificSeals.AddTag(TestOtherSeal);
    Progress.GeneralSeals = 3;
    TestTrue(TEXT("seal: affordable, broken"), Progress.BreakSeal(*Node) && Progress.IsSealBroken(Node->NodeId));
    TestTrue(TEXT("seal: its seals spent, the rest kept"),
        !Progress.SpecificSeals.HasTagExact(TestSeal) && Progress.SpecificSeals.HasTagExact(TestOtherSeal) && Progress.GeneralSeals == 2);
    TestFalse(TEXT("seal: can't be broken twice"), Progress.CanBreakSeal(*Node));

    // --- Still gated by its required action, until shards give a permanent use ---
    TestEqual(TEXT("seal broken, action missing: still gated"), Progress.GetNodeState(*Node, true, Table, Check), EConnectItNodeState::Gated);
    TestTrue(TEXT("seal broken: only the action is missing"), Check.MissingActions.HasTagExact(TestAction) && !Check.IsMissingSeals());
    Progress.ItemUses.Add(TestAction, 3);
    TestEqual(TEXT("item uses don't open it"), Progress.GetNodeState(*Node, true, Table, Check), EConnectItNodeState::Gated);
    Progress.Shards.Add(TestAction, 3);
    TestEqual(TEXT("a permanent use: available"), Progress.GetNodeState(*Node, true, Table, Check), EConnectItNodeState::Available);

    // --- Won: completed ---
    Progress.RecordResult(*Node, true, {}, nullptr);
    TestEqual(TEXT("won: completed"), Progress.GetNodeState(*Node, true, Table, Check), EConnectItNodeState::Completed);
    return true;
}

// Adventure hub plan, Stage 2: a match's result -- the inventory written back
// win or lose, rewards once, items capped, the tag <-> class lookup
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAdventureResultsTest,
    "ConnectIt.Adventure.Results", ConnectItAdventureTests::TestFlags)

bool FConnectItAdventureResultsTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItAdventureTests;
    const TSubclassOf<UTurnBasedPlayerAction> Swap = UConnectIt_SwapPieceAction::StaticClass();
    const TSubclassOf<UTurnBasedPlayerAction> Place = UConnectIt_PlacePieceAction::StaticClass();
    const FGameplayTag SwapTag = UTurnBasedPlayerAction::GetTagForClass(Swap);
    const FGameplayTag PlaceTag = UTurnBasedPlayerAction::GetTagForClass(Place);
    if (!TestTrue(TEXT("the actions have tags"), SwapTag.IsValid() && PlaceTag.IsValid())) return false;

    // Swap can be held (up to 2); Place can't (not listed)
    UItemActionInventoryDataAsset* Items = NewObject<UItemActionInventoryDataAsset>();
    Items->MaxHeld.Add(Swap, 2);

    UConnectIt_ActionShardTableDataAsset* Table = MakeTable();
    FConnectItActionShardCosts PlaceCosts;
    PlaceCosts.ActionClass = Place;
    PlaceCosts.ShardsPerUse = { 2, 2 };
    Table->Actions.Add(PlaceTag, PlaceCosts);

    // --- Tag -> class: the shard table's entry, else the item inventory asset ---
    TestTrue(TEXT("lookup: through the shard table"), FConnectItAdventureProgress::FindActionClass(PlaceTag, Table, Items) == Place);
    TestTrue(TEXT("lookup: through the item caps"), FConnectItAdventureProgress::FindActionClass(SwapTag, Table, Items) == Swap);
    TestTrue(TEXT("lookup: unknown tag, none"), FConnectItAdventureProgress::FindActionClass(TestOtherAction, Table, Items) == nullptr);

    UConnectIt_MapNodeDataAsset* Node = NewObject<UConnectIt_MapNodeDataAsset>();
    Node->Rewards.Shards.Add(PlaceTag, 2);
    Node->Rewards.ItemUses.Add(SwapTag, 5);
    Node->Rewards.ItemUses.Add(PlaceTag, 1);     // can't be held: skipped (warned)
    Node->Rewards.Seals.AddTag(TestSeal);
    Node->Rewards.GeneralSeals = 1;

    FConnectItAdventureProgress Progress;
    Progress.ItemUses.Add(SwapTag, 1);

    // --- A loss: the match's inventory comes back (one spent, none granted); nothing else ---
    TestFalse(TEXT("loss: no rewards"), Progress.RecordResult(*Node, false, {}, Items));
    TestTrue(TEXT("loss: the spent item is gone, nothing completed"),
        Progress.GetItemCount(SwapTag) == 0 && !Progress.IsCompleted(Node->NodeId) && Progress.Shards.IsEmpty());
    TestTrue(TEXT("loss: still the last played"), Progress.LastPlayedNode == Node->NodeId);

    // --- A win: the inventory the match left (a grant kept), then the rewards ---
    FItemActionEntry Granted;
    Granted.ActionClass = Swap;
    Granted.Count = 1;
    TestTrue(TEXT("win: rewards granted"), Progress.RecordResult(*Node, true, { Granted }, Items));
    TestEqual(TEXT("win: items capped at 2 (1 held + 5 rewarded)"), Progress.GetItemCount(SwapTag), 2);
    TestEqual(TEXT("win: an item that can't be held isn't added"), Progress.GetItemCount(PlaceTag), 0);
    TestTrue(TEXT("win: seals and shards"),
        Progress.SpecificSeals.HasTagExact(TestSeal) && Progress.GeneralSeals == 1 && Progress.GetShards(PlaceTag) == 2);
    TestEqual(TEXT("win: shards give a permanent use"), Progress.GetPermanentUses(PlaceTag, Table), 1);

    // --- Won again: never rewarded twice ---
    TestFalse(TEXT("again: no rewards"), Progress.RecordResult(*Node, true, { Granted }, Items));
    TestTrue(TEXT("again: shards and seals unchanged"), Progress.GetShards(PlaceTag) == 2 && Progress.GeneralSeals == 1);
    return true;
}

// Adventure hub plan, Stage 3: an open node's permanent actions, from the
// level's loadout and the shards
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAdventureOpenSetUpTest,
    "ConnectIt.Adventure.OpenSetUp", ConnectItAdventureTests::TestFlags)

bool FConnectItAdventureOpenSetUpTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItAdventureTests;
    const TSubclassOf<UTurnBasedPlayerAction> Place = UConnectIt_PlacePieceAction::StaticClass();
    const TSubclassOf<UTurnBasedPlayerAction> Swap = UConnectIt_SwapPieceAction::StaticClass();
    const TSubclassOf<UTurnBasedPlayerAction> Shift = UConnectIt_BoardShiftAction::StaticClass();

    // The level's loadout: Place (unlimited) and Swap (2 a turn, cooldown 1)
    UActionLoadoutDataAsset* Loadout = NewObject<UActionLoadoutDataAsset>();
    FPermanentActionConfig PlaceEntry;
    PlaceEntry.ActionClass = Place;
    FPermanentActionConfig SwapEntry;
    SwapEntry.ActionClass = Swap;
    SwapEntry.MatchUses = 5;
    SwapEntry.TurnUses = 2;
    SwapEntry.CooldownTurns = 1;
    Loadout->PermanentActions = { PlaceEntry, SwapEntry };

    // Swap and Shift are priced in shards (TestAction too, with no class)
    UConnectIt_ActionShardTableDataAsset* Table = MakeTable();
    const auto Price = [Table](TSubclassOf<UTurnBasedPlayerAction> ActionClass, TArray<int32> Costs)
    {
        FConnectItActionShardCosts Entry;
        Entry.ActionClass = ActionClass;
        Entry.ShardsPerUse = MoveTemp(Costs);
        Table->Actions.Add(UTurnBasedPlayerAction::GetTagForClass(ActionClass), Entry);
    };
    Price(Swap, { 2, 2 });
    Price(Shift, { 3 });

    FConnectItAdventureProgress Progress;
    const auto Find = [](const TArray<FPermanentActionConfig>& Actions, TSubclassOf<UTurnBasedPlayerAction> ActionClass)
    {
        return Actions.FindByPredicate([ActionClass](const FPermanentActionConfig& Entry) { return Entry.ActionClass == ActionClass; });
    };

    // --- No shards: only everyone's actions ---
    {
        const TArray<FPermanentActionConfig> Actions = Progress.BuildOpenPermanentActions(Loadout, Table);
        TestTrue(TEXT("no shards: Place only, as the loadout sets it"),
            Actions.Num() == 1 && Find(Actions, Place) && Find(Actions, Place)->HasUnlimitedMatchUses());
    }

    // --- Shards: Swap 2 uses (its loadout's turn uses / cooldown), Shift 1 (defaults) ---
    Progress.Shards.Add(UTurnBasedPlayerAction::GetTagForClass(Swap), 4);
    Progress.Shards.Add(UTurnBasedPlayerAction::GetTagForClass(Shift), 3);
    {
        const TArray<FPermanentActionConfig> Actions = Progress.BuildOpenPermanentActions(Loadout, Table);
        const FPermanentActionConfig* OpenSwap = Find(Actions, Swap);
        const FPermanentActionConfig* OpenShift = Find(Actions, Shift);
        TestEqual(TEXT("shards: Place, Swap and Shift"), Actions.Num(), 3);
        TestTrue(TEXT("shards: Swap's match uses are its permanent uses; its turn uses and cooldown the loadout's"),
            OpenSwap && OpenSwap->MatchUses == 2 && OpenSwap->TurnUses == 2 && OpenSwap->CooldownTurns == 1);
        TestTrue(TEXT("shards: Shift (not in the loadout) gets 1 use, 1 a turn, no cooldown"),
            OpenShift && OpenShift->MatchUses == 1 && OpenShift->TurnUses == 1 && OpenShift->CooldownTurns == 0);
    }
    return true;
}

// Adventure hub plan, Stage 3: the saved inventory goes into a match and comes
// back -- an item spent is gone, an item granted is kept, even on a loss
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAdventureInventoryTest,
    "ConnectIt.Adventure.Inventory", ConnectItAdventureTests::TestFlags)

bool FConnectItAdventureInventoryTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItAdventureTests;
    const TSubclassOf<UTurnBasedPlayerAction> Swap = UConnectIt_SwapPieceAction::StaticClass();
    const TSubclassOf<UTurnBasedPlayerAction> Shift = UConnectIt_BoardShiftAction::StaticClass();
    const FGameplayTag SwapTag = UTurnBasedPlayerAction::GetTagForClass(Swap);
    const FGameplayTag ShiftTag = UTurnBasedPlayerAction::GetTagForClass(Shift);

    // The game's item caps, for this test only
    UItemActionInventoryDataAsset* Items = NewObject<UItemActionInventoryDataAsset>();
    Items->MaxHeld.Add(Swap, 3);
    Items->MaxHeld.Add(Shift, 2);
    UTurnBasedMechanicsSettings* Settings = GetMutableDefault<UTurnBasedMechanicsSettings>();
    const TSoftObjectPtr<UItemActionInventoryDataAsset> PreviousItems = Settings->ItemInventory;
    Settings->ItemInventory = Items;

    // A throwaway world, so a PlayerState can be spawned with authority
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("ConnectItAdventureTestWorld"));
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
        GetMutableDefault<UTurnBasedMechanicsSettings>()->ItemInventory = PreviousItems;
    };
    ATurnBasedPlayerState* PlayerState = World->SpawnActor<ATurnBasedPlayerState>();
    if (!TestNotNull(TEXT("player state"), PlayerState)) return false;

    // --- Into the match: the saved inventory, by class ---
    FConnectItAdventureProgress Progress;
    Progress.ItemUses.Add(SwapTag, 2);
    PlayerState->InitialiseInventory(Progress.MakeInventory(nullptr, Items));
    TestEqual(TEXT("match: the saved Swaps held"), PlayerState->GetItemCount(Swap), 2);

    // --- The match: one Swap used, one Shift granted ---
    FItemBudget Budget;
    Budget.MatchUses = -1;
    Budget.TurnUses = 2;
    PlayerState->SetItemBudget(Budget);
    TestTrue(TEXT("match: a Swap used"), PlayerState->ConsumeItemUse(Swap));
    TestEqual(TEXT("match: a Shift granted"), PlayerState->GrantItems(Shift, 1), 1);

    // --- Back from a loss: the spent item gone, the grant kept, nothing completed ---
    const UConnectIt_MapNodeDataAsset* Node = MakeGatedNode();
    TestFalse(TEXT("loss: no rewards"), Progress.RecordResult(*Node, false, PlayerState->GetInventory(), Items));
    TestEqual(TEXT("loss: one Swap left"), Progress.GetItemCount(SwapTag), 1);
    TestEqual(TEXT("loss: the granted Shift kept"), Progress.GetItemCount(ShiftTag), 1);
    TestFalse(TEXT("loss: not completed"), Progress.IsCompleted(Node->NodeId));
    return true;
}

#endif
