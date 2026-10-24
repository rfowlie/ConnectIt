// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "NativeGameplayTags.h"
#include "Adventure/ConnectIt_AdventureProgress.h"
#include "Adventure/ConnectIt_MapNodeDataAsset.h"
#include "Adventure/ConnectIt_SkillShardTable.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ConnectItAdventureTests
{
    constexpr EAutomationTestFlags TestFlags =
        EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

    UE_DEFINE_GAMEPLAY_TAG_STATIC(TestSkill, "ConnectIt.Action.Test");
    UE_DEFINE_GAMEPLAY_TAG_STATIC(TestOtherSkill, "ConnectIt.Action.TestOther");
    UE_DEFINE_GAMEPLAY_TAG_STATIC(TestSeal, "ConnectIt.Adventure.Seal.Test");
    UE_DEFINE_GAMEPLAY_TAG_STATIC(TestOtherSeal, "ConnectIt.Adventure.Seal.TestOther");

    // A table where TestSkill's uses cost 3, 5 and 8 shards: uses at 3, 8 and
    // 16 shards in all
    UConnectIt_SkillShardTable* MakeTable()
    {
        UConnectIt_SkillShardTable* Table = NewObject<UConnectIt_SkillShardTable>();
        FConnectItSkillShardCosts Costs;
        Costs.ShardsPerUse = { 3, 5, 8 };
        Table->Skills.Add(TestSkill, Costs);
        return Table;
    }

    // A level node needing TestSkill, TestSeal and one general seal
    UConnectIt_MapNodeDataAsset* MakeGatedNode()
    {
        UConnectIt_MapNodeDataAsset* Node = NewObject<UConnectIt_MapNodeDataAsset>();
        Node->Level = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Test/TestLevel.TestLevel")));
        Node->Gate.RequiredSkills.AddTag(TestSkill);
        Node->Gate.RequiredSeals.AddTag(TestSeal);
        Node->Gate.GeneralSeals = 1;
        return Node;
    }
}

// Shards become numbered uses through a skill's table: each use costs its
// entry's shards on top of the earlier uses'
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAdventureShardTableTest,
    "ConnectIt.Adventure.ShardTable", ConnectItAdventureTests::TestFlags)

bool FConnectItAdventureShardTableTest::RunTest(const FString& Parameters)
{
    const UConnectIt_SkillShardTable* Table = ConnectItAdventureTests::MakeTable();

    TestEqual(TEXT("0 shards: no use"), Table->GetNumberedUses(ConnectItAdventureTests::TestSkill, 0), 0);
    TestEqual(TEXT("2 shards: no use"), Table->GetNumberedUses(ConnectItAdventureTests::TestSkill, 2), 0);
    TestEqual(TEXT("3 shards: 1 use"), Table->GetNumberedUses(ConnectItAdventureTests::TestSkill, 3), 1);
    TestEqual(TEXT("7 shards: still 1 use"), Table->GetNumberedUses(ConnectItAdventureTests::TestSkill, 7), 1);
    TestEqual(TEXT("8 shards (3 + 5): 2 uses"), Table->GetNumberedUses(ConnectItAdventureTests::TestSkill, 8), 2);
    TestEqual(TEXT("15 shards: still 2 uses"), Table->GetNumberedUses(ConnectItAdventureTests::TestSkill, 15), 2);
    TestEqual(TEXT("16 shards (3 + 5 + 8): 3 uses"), Table->GetNumberedUses(ConnectItAdventureTests::TestSkill, 16), 3);
    TestEqual(TEXT("past the table: still 3"), Table->GetNumberedUses(ConnectItAdventureTests::TestSkill, 100), 3);
    TestEqual(TEXT("a skill with no entry: none"), Table->GetNumberedUses(ConnectItAdventureTests::TestOtherSkill, 100), 0);

    // Through a progress value (shards are what's saved; uses are derived)
    FConnectItAdventureProgress Progress;
    Progress.Shards.Add(ConnectItAdventureTests::TestSkill, 5);
    TestEqual(TEXT("progress: 5 shards = 1 use"), Progress.GetNumberedUses(ConnectItAdventureTests::TestSkill, Table), 1);
    TestEqual(TEXT("progress: no table = none"), Progress.GetNumberedUses(ConnectItAdventureTests::TestSkill, nullptr), 0);
    Progress.Shards.Add(ConnectItAdventureTests::TestSkill, 8);
    TestEqual(TEXT("progress: more shards, more uses"), Progress.GetNumberedUses(ConnectItAdventureTests::TestSkill, Table), 2);

    // Falling costs are only a warning, and still count: 5 then 3 = uses at 5 and 8
    UConnectIt_SkillShardTable* Falling = NewObject<UConnectIt_SkillShardTable>();
    FConnectItSkillShardCosts Costs;
    Costs.ShardsPerUse = { 5, 3 };
    Falling->Skills.Add(ConnectItAdventureTests::TestSkill, Costs);
    FDataValidationContext Validation;
    TestTrue(TEXT("falling: still valid"), Falling->IsDataValid(Validation) != EDataValidationResult::Invalid);
    TestEqual(TEXT("falling: one warning"), static_cast<int32>(Validation.GetNumWarnings()), 1);
    TestEqual(TEXT("falling: 7 shards = 1 use"), Falling->GetNumberedUses(ConnectItAdventureTests::TestSkill, 7), 1);
    TestEqual(TEXT("falling: 8 shards = 2 uses"), Falling->GetNumberedUses(ConnectItAdventureTests::TestSkill, 8), 2);
    return true;
}

// What a gated node still needs, against a progress value
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAdventureNodeRequirementsTest,
    "ConnectIt.Adventure.NodeRequirements", ConnectItAdventureTests::TestFlags)

bool FConnectItAdventureNodeRequirementsTest::RunTest(const FString& Parameters)
{
    const UConnectIt_SkillShardTable* Table = ConnectItAdventureTests::MakeTable();
    const UConnectIt_MapNodeDataAsset* Node = ConnectItAdventureTests::MakeGatedNode();

    FConnectItAdventureProgress Progress;
    FConnectItNodeRequirementCheck Check = Node->CheckRequirements(Progress, Table);
    TestFalse(TEXT("nothing yet: not met"), Check.IsMet());
    TestTrue(TEXT("nothing yet: missing the skill"), Check.MissingSkills.HasTagExact(ConnectItAdventureTests::TestSkill));
    TestTrue(TEXT("nothing yet: missing the seal"), Check.MissingSeals.HasTagExact(ConnectItAdventureTests::TestSeal));
    TestEqual(TEXT("nothing yet: one general seal short"), Check.MissingGeneralSeals, 1);

    // Itemed uses never count towards a required skill
    Progress.ItemUses.Add(ConnectItAdventureTests::TestSkill, 5);
    Progress.Shards.Add(ConnectItAdventureTests::TestSkill, 2);
    Check = Node->CheckRequirements(Progress, Table);
    TestTrue(TEXT("itemed uses and too few shards: skill still missing"), Check.MissingSkills.HasTagExact(ConnectItAdventureTests::TestSkill));

    // Enough shards for a numbered use (the 1st costs 3)
    Progress.Shards.Add(ConnectItAdventureTests::TestSkill, 3);
    Check = Node->CheckRequirements(Progress, Table);
    TestTrue(TEXT("a numbered use: skill met"), Check.MissingSkills.IsEmpty());
    TestTrue(TEXT("seals still missing"), Check.IsMissingSeals());

    // The wrong specific seal doesn't open it
    Progress.SpecificSeals.AddTag(ConnectItAdventureTests::TestOtherSeal);
    Progress.GeneralSeals = 1;
    Check = Node->CheckRequirements(Progress, Table);
    TestTrue(TEXT("another seal: still missing this one"), Check.MissingSeals.HasTagExact(ConnectItAdventureTests::TestSeal));
    TestEqual(TEXT("one general seal held: none short"), Check.MissingGeneralSeals, 0);

    Progress.SpecificSeals.AddTag(ConnectItAdventureTests::TestSeal);
    TestTrue(TEXT("skill and seals: met"), Node->CheckRequirements(Progress, Table).IsMet());

    // A broken seal needs no seals any more -- but required skills still count
    FConnectItAdventureProgress Broken;
    Broken.BrokenSeals.Add(Node->NodeId);
    Check = Node->CheckRequirements(Broken, Table);
    TestFalse(TEXT("broken seal: no seals needed"), Check.IsMissingSeals());
    TestTrue(TEXT("broken seal: skill still needed"), Check.MissingSkills.HasTagExact(ConnectItAdventureTests::TestSkill));
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

    FDataValidationContext Valid;
    TestTrue(TEXT("a level node: valid"), Node->IsDataValid(Valid) != EDataValidationResult::Invalid);

    // Neither a level nor a sub-map, and unlocking itself
    UConnectIt_MapNodeDataAsset* Broken = NewObject<UConnectIt_MapNodeDataAsset>();
    Broken->Unlocks.Add(Broken);
    FDataValidationContext Validation;
    TestTrue(TEXT("plays nothing, unlocks itself: invalid"),
        Broken->IsDataValid(Validation) == EDataValidationResult::Invalid);
    TestEqual(TEXT("both reported"), static_cast<int32>(Validation.GetNumErrors()), 2);

    // Both a level and a sub-map
    Node->SubMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Test/TestHub.TestHub")));
    FDataValidationContext Both;
    TestTrue(TEXT("a level and a sub-map: invalid"), Node->IsDataValid(Both) == EDataValidationResult::Invalid);
    return true;
}

#endif
