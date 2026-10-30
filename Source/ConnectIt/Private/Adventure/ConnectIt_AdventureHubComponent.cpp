// Fill out your copyright notice in the Description page of Project Settings.

#include "Adventure/ConnectIt_AdventureHubComponent.h"
#include "Adventure/ConnectIt_MapNodeComponent.h"
#include "Adventure/ConnectIt_MapNodeDataAsset.h"
#include "Adventure/ConnectIt_ActionShardTableDataAsset.h"
#include "Adventure/ConnectIt_AdventureSubsystem.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/World.h"
#include "UObject/UObjectIterator.h"


bool UConnectIt_AdventureHubComponent::bRevealAllNodes = false;

namespace
{
    // ConnectIt.Adventure.RevealAll 1 / 0: every node revealed (worst-case
    // performance, layout work), applied at once to the world's hub
    FAutoConsoleCommandWithWorldAndArgs RevealAllCommand(
        TEXT("ConnectIt.Adventure.RevealAll"),
        TEXT("1 reveals every node of the Adventure hub map, 0 goes back to the player's progress"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
        {
            UConnectIt_AdventureHubComponent::bRevealAllNodes = Args.IsEmpty() || Args[0] != TEXT("0");
            if (UConnectIt_AdventureHubComponent* Hub = UConnectIt_AdventureHubComponent::GetAdventureHub(World))
            {
                Hub->RefreshReveal();
            }
        }));
}

UConnectIt_AdventureHubComponent::UConnectIt_AdventureHubComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

UConnectIt_AdventureHubComponent* UConnectIt_AdventureHubComponent::GetAdventureHub(const UObject* WorldContextObject)
{
    const UWorld* World = IsValid(WorldContextObject) ? WorldContextObject->GetWorld() : nullptr;
    const AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
    return GameMode ? GameMode->FindComponentByClass<UConnectIt_AdventureHubComponent>() : nullptr;
}

void UConnectIt_AdventureHubComponent::BeginPlay()
{
    Super::BeginPlay();
    DiscoverNodes();

    if (UConnectIt_AdventureSubsystem* Adventure =
        GetWorld() && GetWorld()->GetGameInstance() ? GetWorld()->GetGameInstance()->GetSubsystem<UConnectIt_AdventureSubsystem>() : nullptr)
    {
        Adventure->OnProgressChanged.AddUniqueDynamic(this, &UConnectIt_AdventureHubComponent::HandleProgressChanged);
    }
    RefreshReveal();
}

void UConnectIt_AdventureHubComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UConnectIt_AdventureSubsystem* Adventure =
        GetWorld() && GetWorld()->GetGameInstance() ? GetWorld()->GetGameInstance()->GetSubsystem<UConnectIt_AdventureSubsystem>() : nullptr)
    {
        Adventure->OnProgressChanged.RemoveDynamic(this, &UConnectIt_AdventureHubComponent::HandleProgressChanged);
    }
    Super::EndPlay(EndPlayReason);
}

void UConnectIt_AdventureHubComponent::HandleProgressChanged()
{
    RefreshReveal();
}

FConnectItRevealWalkResult UConnectIt_AdventureHubComponent::RunRevealWalk(const UConnectIt_MapNodeDataAsset* Start,
    const TSet<const UConnectIt_MapNodeDataAsset*>& MapNodes, const FConnectItAdventureProgress& Progress,
    const UConnectIt_ActionShardTableDataAsset* Table, bool bRevealAll)
{
    FConnectItRevealWalkResult Result;
    FConnectItNodeRequirementCheck Check;
    const auto Reveal = [&](const UConnectIt_MapNodeDataAsset* Node)
    {
        Result.States.Add(const_cast<UConnectIt_MapNodeDataAsset*>(Node), Progress.GetNodeState(*Node, true, Table, Check));
    };
    const auto AddLinksFrom = [&](const UConnectIt_MapNodeDataAsset* Node)
    {
        for (const TSoftObjectPtr<UConnectIt_MapNodeDataAsset>& Unlock : Node->Unlocks)
        {
            const UConnectIt_MapNodeDataAsset* To = Unlock.Get();
            if (To && Result.States.Contains(To))
            {
                FConnectItRevealedLink& Link = Result.Links.AddDefaulted_GetRef();
                Link.From = const_cast<UConnectIt_MapNodeDataAsset*>(Node);
                Link.To = const_cast<UConnectIt_MapNodeDataAsset*>(To);
            }
        }
    };

    // Debug: everything in the map
    if (bRevealAll)
    {
        for (const UConnectIt_MapNodeDataAsset* Node : MapNodes)
        {
            if (Node) Reveal(Node);
        }
        for (const UConnectIt_MapNodeDataAsset* Node : MapNodes)
        {
            if (Node) AddLinksFrom(Node);
        }
        return Result;
    }

    if (!Start || !MapNodes.Contains(Start)) return Result;

    // Breadth-first through completed nodes' unlocks; each node once
    TArray<const UConnectIt_MapNodeDataAsset*> Queue = { Start };
    TSet<const UConnectIt_MapNodeDataAsset*> Visited = { Start };
    Reveal(Start);
    for (int32 Index = 0; Index < Queue.Num(); Index++)
    {
        const UConnectIt_MapNodeDataAsset* Node = Queue[Index];
        if (Result.GetState(Node) != EConnectItNodeState::Completed) continue;   // the frontier

        for (const TSoftObjectPtr<UConnectIt_MapNodeDataAsset>& Unlock : Node->Unlocks)
        {
            // Only this map's nodes (loaded: their components hold them)
            const UConnectIt_MapNodeDataAsset* Next = Unlock.Get();
            if (!Next || !MapNodes.Contains(Next) || Visited.Contains(Next)) continue;
            Visited.Add(Next);
            Reveal(Next);
            Queue.Add(Next);
        }
    }

    // The links between revealed nodes, from completed nodes (what they revealed)
    for (const UConnectIt_MapNodeDataAsset* Node : Queue)
    {
        if (Result.GetState(Node) == EConnectItNodeState::Completed) AddLinksFrom(Node);
    }
    return Result;
}

void UConnectIt_AdventureHubComponent::RefreshReveal()
{
    // The player's progress (the game instance loads it before any map: an
    // empty progress here means a fresh save -- the start only)
    const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const UConnectIt_AdventureSubsystem* Adventure = GameInstance ? GameInstance->GetSubsystem<UConnectIt_AdventureSubsystem>() : nullptr;
    if (!Adventure)
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectIt_AdventureHubComponent: no Adventure progress -- only the start is revealed"));
    }
    const FConnectItAdventureProgress Progress = Adventure ? Adventure->GetProgress() : FConnectItAdventureProgress();

    TSet<const UConnectIt_MapNodeDataAsset*> MapNodes;
    for (const TPair<TObjectPtr<UConnectIt_MapNodeDataAsset>, TObjectPtr<UConnectIt_MapNodeComponent>>& Pair : NodeToComponent)
    {
        MapNodes.Add(Pair.Key);
    }

    RevealResult = RunRevealWalk(StartNode ? StartNode->NodeData.Get() : nullptr, MapNodes, Progress,
        UConnectIt_ActionShardTableDataAsset::Get(), bRevealAllNodes);
    bWalked = true;

    for (const TPair<TObjectPtr<UConnectIt_MapNodeDataAsset>, TObjectPtr<UConnectIt_MapNodeComponent>>& Pair : NodeToComponent)
    {
        if (IsValid(Pair.Value)) Pair.Value->SetState(RevealResult.GetState(Pair.Key));
    }

    UE_LOG(LogTemp, Log, TEXT("ConnectIt_AdventureHubComponent: %d of %d node(s) revealed, %d link(s)%s"),
        RevealResult.States.Num(), NodeToComponent.Num(), RevealResult.Links.Num(),
        bRevealAllNodes ? TEXT(" (reveal all)") : TEXT(""));
}

void UConnectIt_AdventureHubComponent::DiscoverNodes()
{
    if (bDiscovered) return;
    bDiscovered = true;

    const UWorld* World = GetWorld();
    for (TObjectIterator<UConnectIt_MapNodeComponent> It; It; ++It)
    {
        UConnectIt_MapNodeComponent* Node = *It;
        if (IsValid(Node) && !Node->IsTemplate() && Node->GetWorld() == World && IsValid(Node->GetOwner()))
        {
            AddNode(Node);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("ConnectIt_AdventureHubComponent: found %d map node(s)%s"), NodeToComponent.Num(),
        StartNode ? TEXT("") : TEXT(" -- none is flagged Start"));
}

void UConnectIt_AdventureHubComponent::AddNode(UConnectIt_MapNodeComponent* Node)
{
    if (!IsValid(Node) || !IsValid(Node->GetOwner())) return;

    UConnectIt_MapNodeDataAsset* NodeData = Node->NodeData;
    if (!NodeData)
    {
        UE_LOG(LogTemp, Warning, TEXT("ConnectIt_AdventureHubComponent: map node on %s has no Node Data -- skipped"),
            *Node->GetOwner()->GetName());
        return;
    }

    // One actor per node: a second node naming the same data asset is rejected
    const TObjectPtr<UConnectIt_MapNodeComponent>* Existing = NodeToComponent.Find(NodeData);
    if (Existing && *Existing != Node && IsValid(*Existing))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AdventureHubComponent: %s and %s are both node %s -- %s is ignored"),
            *(*Existing)->GetOwner()->GetName(), *Node->GetOwner()->GetName(), *NodeData->GetName(),
            *Node->GetOwner()->GetName());
        return;
    }

    // Add-or-update: a node whose data asset changed drops its old entry
    if (const TObjectPtr<UConnectIt_MapNodeDataAsset>* Previous = ActorToNode.Find(Node->GetOwner()))
    {
        if (*Previous != NodeData) NodeToComponent.Remove(*Previous);
    }
    NodeToComponent.Add(NodeData, Node);
    ActorToNode.Add(Node->GetOwner(), NodeData);

    // A late arrival takes its state from the last walk (Hidden if it didn't
    // reach it)
    if (bWalked)
    {
        Node->SetState(RevealResult.GetState(NodeData));
    }

    if (Node->bIsStart)
    {
        if (StartNode && StartNode != Node)
        {
            UE_LOG(LogTemp, Error, TEXT("ConnectIt_AdventureHubComponent: more than one node is flagged Start (%s, %s) -- "
                "the first is used"), *StartNode->GetOwner()->GetName(), *Node->GetOwner()->GetName());
        }
        else
        {
            StartNode = Node;
        }
    }
    if (Node->bIsExit)
    {
        if (ExitNode && ExitNode != Node)
        {
            UE_LOG(LogTemp, Error, TEXT("ConnectIt_AdventureHubComponent: more than one node is flagged Exit (%s, %s) -- "
                "the first is used"), *ExitNode->GetOwner()->GetName(), *Node->GetOwner()->GetName());
        }
        else
        {
            ExitNode = Node;
        }
    }
}

void UConnectIt_AdventureHubComponent::RemoveNode(UConnectIt_MapNodeComponent* Node)
{
    if (!Node) return;

    for (auto It = NodeToComponent.CreateIterator(); It; ++It)
    {
        if (It.Value() == Node) It.RemoveCurrent();
    }
    for (auto It = ActorToNode.CreateIterator(); It; ++It)
    {
        if (It.Key() == Node->GetOwner()) It.RemoveCurrent();
    }
    if (StartNode == Node) StartNode = nullptr;
    if (ExitNode == Node) ExitNode = nullptr;
}

AActor* UConnectIt_AdventureHubComponent::GetActorForNode(const UConnectIt_MapNodeDataAsset* NodeData) const
{
    const UConnectIt_MapNodeComponent* Node = GetNodeComponent(NodeData);
    return Node ? Node->GetOwner() : nullptr;
}

UConnectIt_MapNodeDataAsset* UConnectIt_AdventureHubComponent::GetNodeForActor(const AActor* Actor) const
{
    const TObjectPtr<UConnectIt_MapNodeDataAsset>* Found = ActorToNode.Find(Actor);
    return Found ? Found->Get() : nullptr;
}

UConnectIt_MapNodeComponent* UConnectIt_AdventureHubComponent::GetNodeComponent(const UConnectIt_MapNodeDataAsset* NodeData) const
{
    const TObjectPtr<UConnectIt_MapNodeComponent>* Found = NodeToComponent.Find(NodeData);
    return Found && IsValid(*Found) ? Found->Get() : nullptr;
}
