// Fill out your copyright notice in the Description page of Project Settings.

#include "Adventure/ConnectIt_AdventureHubComponent.h"
#include "Adventure/ConnectIt_MapNodeComponent.h"
#include "Adventure/ConnectIt_MapNodeDataAsset.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/World.h"
#include "UObject/UObjectIterator.h"


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
