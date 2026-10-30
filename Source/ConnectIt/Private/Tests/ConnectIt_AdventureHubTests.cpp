// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Adventure/ConnectIt_AdventureHubComponent.h"
#include "Adventure/ConnectIt_MapNodeComponent.h"
#include "Adventure/ConnectIt_MapNodeDataAsset.h"
#include "Adventure/ConnectIt_HubLinkActor.h"
#include "Framework/GameMode/ConnectIt_GameMode_AdventureHub.h"
#include "Animation/TransformLerpComponent.h"
#include "Hover/GridHoverComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
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

// Adventure hub map plan, Stage 2: the reveal walk on a small graph
//   S -> A, B;  A -> C (gated by a seal), D (not in this map);  B -> S (a loop)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAdventureRevealWalkTest,
    "ConnectIt.Adventure.RevealWalk", ConnectItAdventureHubTests::TestFlags)

bool FConnectItAdventureRevealWalkTest::RunTest(const FString& Parameters)
{
    using EState = EConnectItNodeState;
    UConnectIt_MapNodeDataAsset* S = NewObject<UConnectIt_MapNodeDataAsset>();
    UConnectIt_MapNodeDataAsset* A = NewObject<UConnectIt_MapNodeDataAsset>();
    UConnectIt_MapNodeDataAsset* B = NewObject<UConnectIt_MapNodeDataAsset>();
    UConnectIt_MapNodeDataAsset* C = NewObject<UConnectIt_MapNodeDataAsset>();
    UConnectIt_MapNodeDataAsset* D = NewObject<UConnectIt_MapNodeDataAsset>();
    S->Unlocks = { A, B };
    A->Unlocks = { C, D };
    B->Unlocks = { S };
    C->Gate.GeneralSeals = 1;
    const TSet<const UConnectIt_MapNodeDataAsset*> MapNodes = { S, A, B, C };
    const auto Walk = [&](const FConnectItAdventureProgress& Progress, bool bRevealAll = false)
    {
        return UConnectIt_AdventureHubComponent::RunRevealWalk(S, MapNodes, Progress, nullptr, bRevealAll);
    };
    const auto HasLink = [](const FConnectItRevealWalkResult& Result, const UConnectIt_MapNodeDataAsset* From, const UConnectIt_MapNodeDataAsset* To)
    {
        return Result.Links.ContainsByPredicate([From, To](const FConnectItRevealedLink& Link) { return Link.From == From && Link.To == To; });
    };

    // --- A fresh save: the start only ---
    FConnectItAdventureProgress Progress;
    Progress.CompletedNodes.Add(FGuid::NewGuid());   // an id no node has (a deleted node): ignored
    {
        const FConnectItRevealWalkResult Result = Walk(Progress);
        TestTrue(TEXT("fresh: the start available, nothing else"),
            Result.States.Num() == 1 && Result.GetState(S) == EState::Available && Result.GetState(A) == EState::Hidden);
        TestEqual(TEXT("fresh: no links"), Result.Links.Num(), 0);
    }

    // --- The start completed: its unlocks revealed (the frontier), the loop back to it harmless ---
    Progress.CompletedNodes.Add(S->NodeId);
    Progress.CompletedNodes.Add(B->NodeId);
    {
        const FConnectItRevealWalkResult Result = Walk(Progress);
        TestTrue(TEXT("start won: S completed, A available, B completed"),
            Result.GetState(S) == EState::Completed && Result.GetState(A) == EState::Available && Result.GetState(B) == EState::Completed);
        TestEqual(TEXT("start won: C still hidden (A not won)"), Result.GetState(C), EState::Hidden);
        TestTrue(TEXT("start won: links S->A, S->B, and the loop B->S"),
            HasLink(Result, S, A) && HasLink(Result, S, B) && HasLink(Result, B, S) && Result.Links.Num() == 3);
    }

    // --- A won: C revealed but gated; D (not in this map) skipped ---
    Progress.CompletedNodes.Add(A->NodeId);
    {
        const FConnectItRevealWalkResult Result = Walk(Progress);
        TestEqual(TEXT("A won: C gated"), Result.GetState(C), EState::Gated);
        TestTrue(TEXT("A won: link A->C, nothing to D"), HasLink(Result, A, C) && !HasLink(Result, A, D) && Result.GetState(D) == EState::Hidden);
    }

    // --- C's seal broken: available ---
    Progress.BrokenSeals.Add(C->NodeId);
    TestEqual(TEXT("seal broken: C available"), Walk(Progress).GetState(C), EState::Available);

    // --- Reveal all (debug): every node of the map, even from a fresh save ---
    {
        const FConnectItRevealWalkResult Result = Walk(FConnectItAdventureProgress(), true);
        TestEqual(TEXT("reveal all: the four map nodes"), Result.States.Num(), 4);
        TestEqual(TEXT("reveal all: C still gated"), Result.GetState(C), EState::Gated);
    }
    return true;
}

// Adventure hub map plan, Stage 3: a hub map's checks (map validation)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAdventureHubValidationTest,
    "ConnectIt.Adventure.HubValidation", ConnectItAdventureHubTests::TestFlags)

bool FConnectItAdventureHubValidationTest::RunTest(const FString& Parameters)
{
    // Node components on throwaway actors (no world needed for the checks)
    const auto MakeNode = [](UConnectIt_MapNodeDataAsset* NodeData, bool bIsStart = false, bool bIsExit = false)
    {
        UConnectIt_MapNodeComponent* Node = NewObject<UConnectIt_MapNodeComponent>(NewObject<AActor>());
        Node->NodeData = NodeData;
        Node->bIsStart = bIsStart;
        Node->bIsExit = bIsExit;
        return Node;
    };
    const auto Check = [](std::initializer_list<const UConnectIt_MapNodeComponent*> Nodes, bool bIsSubMap = false)
    {
        return UConnectIt_AdventureHubComponent::CheckHubMapNodes(TArray<const UConnectIt_MapNodeComponent*>(Nodes), bIsSubMap);
    };

    // S -> A -> B, and B is the exit
    UConnectIt_MapNodeDataAsset* S = NewObject<UConnectIt_MapNodeDataAsset>();
    UConnectIt_MapNodeDataAsset* A = NewObject<UConnectIt_MapNodeDataAsset>();
    UConnectIt_MapNodeDataAsset* B = NewObject<UConnectIt_MapNodeDataAsset>();
    S->Unlocks = { A };
    A->Unlocks = { B };
    const UConnectIt_MapNodeComponent* NodeS = MakeNode(S, true);
    const UConnectIt_MapNodeComponent* NodeA = MakeNode(A);
    const UConnectIt_MapNodeComponent* NodeB = MakeNode(B, false, true);

    {
        const FConnectItHubMapIssues Issues = Check({ NodeS, NodeA, NodeB }, true);
        TestTrue(TEXT("a good sub-map: no errors, no warnings"), Issues.Errors.IsEmpty() && Issues.Warnings.IsEmpty());
    }
    TestEqual(TEXT("no Start: an error"), Check({ NodeA, NodeB }).Errors.Num(), 1);
    TestEqual(TEXT("two Starts: an error"), Check({ NodeS, NodeA, MakeNode(B, true) }).Errors.Num(), 1);
    TestEqual(TEXT("two Exits: an error"), Check({ NodeS, NodeA, NodeB, MakeNode(NewObject<UConnectIt_MapNodeDataAsset>(), false, true) }).Errors.Num(), 1);
    TestEqual(TEXT("one node on two actors: an error"), Check({ NodeS, NodeA, NodeB, MakeNode(A) }).Errors.Num(), 1);
    TestEqual(TEXT("a sub-map with no Exit: an error"), Check({ NodeS, NodeA, MakeNode(B) }, true).Errors.Num(), 1);
    TestTrue(TEXT("not a sub-map: no Exit needed"), Check({ NodeS, NodeA, MakeNode(B) }).Errors.IsEmpty());

    // Warnings: an unlock with no node here; a node nothing leads to; a node with no data
    {
        const FConnectItHubMapIssues Issues = Check({ NodeS, NodeA });   // A unlocks B, which isn't here
        TestTrue(TEXT("unlock off the map: a warning"), Issues.Errors.IsEmpty() && Issues.Warnings.Num() == 1);
    }
    {
        const FConnectItHubMapIssues Issues = Check({ NodeS, NodeA, NodeB, MakeNode(NewObject<UConnectIt_MapNodeDataAsset>()) });
        TestEqual(TEXT("unreachable from the Start: a warning"), Issues.Warnings.Num(), 1);
    }
    TestEqual(TEXT("no node data: a warning"), Check({ NodeS, NodeA, NodeB, MakeNode(nullptr) }).Warnings.Num(), 1);
    return true;
}

// Adventure hub map plan, Stage 4: a node scales up while hovered and back
// after; hover and click reach the hub (hidden nodes ignored); the reveal's
// links become link actors between the right nodes
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAdventureHubInteractionTest,
    "ConnectIt.Adventure.HubInteraction", ConnectItAdventureHubTests::TestFlags)

bool FConnectItAdventureHubInteractionTest::RunTest(const FString& Parameters)
{
    using namespace ConnectItAdventureHubTests;
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("ConnectItAdventureHubInteractionTestWorld"));
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
    };

    AConnectIt_GameMode_AdventureHub* GameMode = World->SpawnActor<AConnectIt_GameMode_AdventureHub>();
    UConnectIt_AdventureHubComponent* Hub = GameMode ? GameMode->GetAdventureHub() : nullptr;
    if (!TestNotNull(TEXT("the hub game mode has the hub"), Hub)) return false;

    UConnectIt_MapNodeDataAsset* S = NewObject<UConnectIt_MapNodeDataAsset>();
    UConnectIt_MapNodeDataAsset* A = NewObject<UConnectIt_MapNodeDataAsset>();
    UConnectIt_MapNodeDataAsset* B = NewObject<UConnectIt_MapNodeDataAsset>();
    UConnectIt_MapNodeComponent* NodeS = SpawnNode(World, S, /*bIsStart=*/true);
    UConnectIt_MapNodeComponent* NodeA = SpawnNode(World, A);
    UConnectIt_MapNodeComponent* NodeB = SpawnNode(World, B);

    // Node A gets a root to scale and a lerp; it sits away from the start
    AActor* ActorA = NodeA->GetOwner();
    USceneComponent* Root = NewObject<USceneComponent>(ActorA);
    ActorA->SetRootComponent(Root);
    Root->RegisterComponent();
    ActorA->SetActorLocation(FVector(400.f, 0.f, 0.f));
    UTransformLerpComponent* Lerp = NewObject<UTransformLerpComponent>(ActorA);
    Lerp->Duration = 0.2f;
    Lerp->RegisterComponent();

    Hub->DiscoverNodes();
    UGridHoverComponent* HoverA = ActorA->FindComponentByClass<UGridHoverComponent>();
    UGridHoverComponent* HoverB = NodeB->GetOwner()->FindComponentByClass<UGridHoverComponent>();
    if (!TestTrue(TEXT("discovery: each node got a hover component"), HoverA && HoverB)) return false;

    // --- The reveal: S won, A available, B hidden; one link S -> A ---
    FConnectItRevealWalkResult Result;
    Result.States.Add(S, EConnectItNodeState::Completed);
    Result.States.Add(A, EConnectItNodeState::Available);
    FConnectItRevealedLink& Link = Result.Links.AddDefaulted_GetRef();
    Link.From = S;
    Link.To = A;
    Hub->ApplyReveal(Result);
    TestEqual(TEXT("reveal: B hidden"), NodeB->GetState(), EConnectItNodeState::Hidden);
    const TArray<AConnectIt_HubLinkActor*> Links = Hub->GetLinkActors();
    if (TestEqual(TEXT("links: one link actor"), Links.Num(), 1))
    {
        const USplineComponent* Spline = Links[0]->FindComponentByClass<USplineComponent>();
        TestTrue(TEXT("links: S to A"), Spline && Spline->GetNumberOfSplinePoints() == 2
            && Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World).Equals(NodeS->GetOwner()->GetActorLocation())
            && Spline->GetLocationAtSplinePoint(1, ESplineCoordinateSpace::World).Equals(ActorA->GetActorLocation()));
    }

    // --- Hover A: the hub knows, the node scales up over the duration ---
    HoverA->NotifyHoverBegin();
    TestTrue(TEXT("hover: A hovered"), Hub->GetHoveredNode() == NodeA);
    for (int32 Step = 0; Step < 5; ++Step) Lerp->TickComponent(0.05f, LEVELTICK_All, nullptr);
    TestEqual(TEXT("hover: fully up"), Lerp->GetAlpha(), 1.f);
    TestTrue(TEXT("hover: scaled 15%"), Root->GetRelativeScale3D().Equals(FVector(1.15f)));

    // --- Unhover: back to the start ---
    HoverA->NotifyHoverEnd();
    TestNull(TEXT("unhover: nothing hovered"), Hub->GetHoveredNode());
    for (int32 Step = 0; Step < 5; ++Step) Lerp->TickComponent(0.05f, LEVELTICK_All, nullptr);
    TestEqual(TEXT("unhover: back down"), Lerp->GetAlpha(), 0.f);
    TestTrue(TEXT("unhover: original scale"), Root->GetRelativeScale3D().Equals(FVector(1.f)));

    // --- Click A selects it; a hidden node is ignored ---
    HoverA->NotifyClicked();
    TestTrue(TEXT("click: A selected"), Hub->GetSelectedNode() == NodeA);
    HoverB->NotifyHoverBegin();
    HoverB->NotifyClicked();
    TestNull(TEXT("hidden: B never hovered"), Hub->GetHoveredNode());
    TestTrue(TEXT("hidden: A still selected"), Hub->GetSelectedNode() == NodeA);

    // --- A new reveal replaces the link actors ---
    Result.Links.Reset();
    Hub->ApplyReveal(Result);
    TestEqual(TEXT("links: rebuilt (none now)"), Hub->GetLinkActors().Num(), 0);
    return true;
}

// Adventure hub map plan, Stage 5: a sub-map node completes on its own (its
// map's Exit won) -- rewards once, the inventory otherwise untouched
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConnectItAdventureSubMapCompleteTest,
    "ConnectIt.Adventure.SubMapComplete", ConnectItAdventureHubTests::TestFlags)

bool FConnectItAdventureSubMapCompleteTest::RunTest(const FString& Parameters)
{
    UConnectIt_MapNodeDataAsset* SubMapNode = NewObject<UConnectIt_MapNodeDataAsset>();
    SubMapNode->Rewards.GeneralSeals = 2;

    FConnectItAdventureProgress Progress;
    TestTrue(TEXT("first: completed with rewards"), Progress.CompleteNode(*SubMapNode, nullptr));
    TestTrue(TEXT("first: completed"), Progress.IsCompleted(SubMapNode->NodeId));
    TestEqual(TEXT("first: seals granted"), Progress.GeneralSeals, 2);
    TestFalse(TEXT("first: not the last played (the caller sets that)"), Progress.LastPlayedNode == SubMapNode->NodeId);

    TestFalse(TEXT("again: nothing"), Progress.CompleteNode(*SubMapNode, nullptr));
    TestEqual(TEXT("again: seals once"), Progress.GeneralSeals, 2);

    // A win recorded on an already-complete node grants nothing either
    TestFalse(TEXT("record after: nothing"), Progress.RecordResult(*SubMapNode, true, {}, nullptr));
    TestEqual(TEXT("record after: seals once"), Progress.GeneralSeals, 2);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
