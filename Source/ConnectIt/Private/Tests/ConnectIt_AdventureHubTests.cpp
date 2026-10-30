// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Adventure/ConnectIt_AdventureHubComponent.h"
#include "Adventure/ConnectIt_MapNodeComponent.h"
#include "Adventure/ConnectIt_MapNodeDataAsset.h"
#include "Framework/GameMode/ConnectIt_GameMode_AdventureHub.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ConnectItAdventureHubTests
{
    constexpr EAutomationTestFlags TestFlags =
        EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

    // An actor dressed as a map node for NodeData
    UConnectIt_MapNodeComponent* SpawnNode(UWorld* World, UConnectIt_MapNodeDataAsset* NodeData, bool bIsStart = false)
    {
        AActor* Actor = World->SpawnActor<AActor>();
        UConnectIt_MapNodeComponent* Node = NewObject<UConnectIt_MapNodeComponent>(Actor);
        Node->NodeData = NodeData;
        Node->bIsStart = bIsStart;
        Node->RegisterComponent();
        return Node;
    }
}

// Adventure hub map plan, Stage 1: the hub finds the map's nodes and keeps
// actor <-> node both ways; duplicates, late arrivals and removal
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAdventureHubDiscoveryTest,
    "ConnectIt.Adventure.HubDiscovery", ConnectItAdventureHubTests::TestFlags)

bool FConnectItAdventureHubDiscoveryTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItAdventureHubTests;
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("ConnectItAdventureHubTestWorld"));
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
    };

    AConnectIt_GameMode_AdventureHub* GameMode = World->SpawnActor<AConnectIt_GameMode_AdventureHub>();
    UConnectIt_AdventureHubComponent* Hub = GameMode ? GameMode->GetAdventureHub() : nullptr;
    if (!TestNotNull(TEXT("the hub game mode has the hub"), Hub)) return false;

    UConnectIt_MapNodeDataAsset* A = NewObject<UConnectIt_MapNodeDataAsset>();
    UConnectIt_MapNodeDataAsset* B = NewObject<UConnectIt_MapNodeDataAsset>();
    UConnectIt_MapNodeDataAsset* C = NewObject<UConnectIt_MapNodeDataAsset>();
    UConnectIt_MapNodeDataAsset* D = NewObject<UConnectIt_MapNodeDataAsset>();
    UConnectIt_MapNodeComponent* NodeA = SpawnNode(World, A, /*bIsStart=*/true);
    UConnectIt_MapNodeComponent* NodeB = SpawnNode(World, B);
    UConnectIt_MapNodeComponent* NodeC = SpawnNode(World, C);
    UConnectIt_MapNodeComponent* SecondA = SpawnNode(World, A);   // the same node twice: one is rejected
    SpawnNode(World, nullptr);                                    // no node data: skipped

    // --- Discovery: every node, both ways; the start ---
    AddExpectedError(TEXT("are both node"), EAutomationExpectedErrorFlags::Contains, 1);
    Hub->DiscoverNodes();
    TestTrue(TEXT("discovery: done"), Hub->HasDiscovered());
    TestEqual(TEXT("discovery: three nodes (a duplicate and a node with no data left out)"), Hub->GetNodeCount(), 3);
    TestTrue(TEXT("discovery: node -> actor"), Hub->GetActorForNode(B) == NodeB->GetOwner() && Hub->GetActorForNode(C) == NodeC->GetOwner());
    TestTrue(TEXT("discovery: actor -> node"), Hub->GetNodeForActor(NodeB->GetOwner()) == B);
    TestTrue(TEXT("discovery: the start"), Hub->GetStartNode() == NodeA || Hub->GetStartNode() == SecondA);

    // The duplicate: one of the two actors is node A, the other is nothing
    const AActor* ActorForA = Hub->GetActorForNode(A);
    const AActor* Rejected = ActorForA == NodeA->GetOwner() ? SecondA->GetOwner() : NodeA->GetOwner();
    TestTrue(TEXT("duplicate: one actor is node A"), ActorForA == NodeA->GetOwner() || ActorForA == SecondA->GetOwner());
    TestNull(TEXT("duplicate: the other isn't a node"), Hub->GetNodeForActor(Rejected));

    // --- A late node pushes itself (twice): added once ---
    UConnectIt_MapNodeComponent* NodeD = SpawnNode(World, D);
    Hub->AddNode(NodeD);
    Hub->AddNode(NodeD);
    TestEqual(TEXT("late: added once"), Hub->GetNodeCount(), 4);
    TestTrue(TEXT("late: both ways"), Hub->GetActorForNode(D) == NodeD->GetOwner() && Hub->GetNodeForActor(NodeD->GetOwner()) == D);

    // --- Removal clears both ways ---
    Hub->RemoveNode(NodeD);
    TestEqual(TEXT("removed: back to three"), Hub->GetNodeCount(), 3);
    TestTrue(TEXT("removed: neither way"), !Hub->GetActorForNode(D) && !Hub->GetNodeForActor(NodeD->GetOwner()));

    // --- Hidden hides the actor; any other state shows it ---
    NodeB->SetState(EConnectItNodeState::Available);
    TestFalse(TEXT("state: available shows the actor"), NodeB->GetOwner()->IsHidden());
    NodeB->SetState(EConnectItNodeState::Hidden);
    TestTrue(TEXT("state: hidden hides it"), NodeB->GetOwner()->IsHidden());
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
