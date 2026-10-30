// Fill out your copyright notice in the Description page of Project Settings.

#include "Adventure/ConnectIt_AdventureHubComponent.h"
#include "Adventure/ConnectIt_MapNodeComponent.h"
#include "Adventure/ConnectIt_MapNodeDataAsset.h"
#include "Adventure/ConnectIt_ActionShardTableDataAsset.h"
#include "Adventure/ConnectIt_AdventureSubsystem.h"
#include "Engine/GameInstance.h"
#include "Adventure/ConnectIt_HubLinkActor.h"
#include "Animation/TransformLerpComponent.h"
#include "Hover/GridHoverComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
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
    FocusLastPlayed();
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

FConnectItHubMapIssues UConnectIt_AdventureHubComponent::CheckHubMapNodes(
    const TArray<const UConnectIt_MapNodeComponent*>& Nodes, bool bIsSubMap)
{
    FConnectItHubMapIssues Issues;
    const auto OwnerName = [](const UConnectIt_MapNodeComponent* Node)
    {
        return Node->GetOwner() ? Node->GetOwner()->GetActorNameOrLabel() : Node->GetName();
    };

    TMap<const UConnectIt_MapNodeDataAsset*, const UConnectIt_MapNodeComponent*> ByData;
    TArray<const UConnectIt_MapNodeComponent*> Starts;
    TArray<const UConnectIt_MapNodeComponent*> Exits;
    for (const UConnectIt_MapNodeComponent* Node : Nodes)
    {
        if (!Node) continue;
        if (Node->bIsStart) Starts.Add(Node);
        if (Node->bIsExit) Exits.Add(Node);

        if (!Node->NodeData)
        {
            Issues.Warnings.Add(FString::Printf(TEXT("map node on %s has no Node Data."), *OwnerName(Node)));
            continue;
        }
        if (const UConnectIt_MapNodeComponent* const* First = ByData.Find(Node->NodeData))
        {
            Issues.Errors.Add(FString::Printf(TEXT("%s and %s are both node %s -- one actor per node."),
                *OwnerName(*First), *OwnerName(Node), *Node->NodeData->GetName()));
            continue;
        }
        ByData.Add(Node->NodeData, Node);
    }

    if (Starts.IsEmpty())
    {
        Issues.Errors.Add(TEXT("no map node is flagged Start -- the reveal walk has nowhere to begin."));
    }
    else if (Starts.Num() > 1)
    {
        Issues.Errors.Add(FString::Printf(TEXT("%d map nodes are flagged Start (%s, %s...) -- one per map."),
            Starts.Num(), *OwnerName(Starts[0]), *OwnerName(Starts[1])));
    }
    if (Exits.Num() > 1)
    {
        Issues.Errors.Add(FString::Printf(TEXT("%d map nodes are flagged Exit (%s, %s...) -- one per map."),
            Exits.Num(), *OwnerName(Exits[0]), *OwnerName(Exits[1])));
    }
    if (bIsSubMap && Exits.IsEmpty())
    {
        Issues.Errors.Add(TEXT("this map is a sub-map (a node elsewhere names it) but no map node is flagged Exit -- "
            "it can never be completed."));
    }

    // Unlocks that point outside this map
    for (const TPair<const UConnectIt_MapNodeDataAsset*, const UConnectIt_MapNodeComponent*>& Pair : ByData)
    {
        for (const TSoftObjectPtr<UConnectIt_MapNodeDataAsset>& Unlock : Pair.Key->Unlocks)
        {
            const UConnectIt_MapNodeDataAsset* Target = Unlock.LoadSynchronous();
            if (Target && !ByData.Contains(Target))
            {
                Issues.Warnings.Add(FString::Printf(TEXT("%s unlocks %s, which has no node in this map -- it can't be "
                    "revealed here."), *Pair.Key->GetName(), *Target->GetName()));
            }
        }
    }

    // Reachability from the Start, through every Unlocks (whatever the progress)
    const UConnectIt_MapNodeDataAsset* StartData = Starts.Num() == 1 ? Starts[0]->NodeData.Get() : nullptr;
    if (StartData)
    {
        TSet<const UConnectIt_MapNodeDataAsset*> Reached = { StartData };
        TArray<const UConnectIt_MapNodeDataAsset*> Queue = { StartData };
        for (int32 Index = 0; Index < Queue.Num(); Index++)
        {
            for (const TSoftObjectPtr<UConnectIt_MapNodeDataAsset>& Unlock : Queue[Index]->Unlocks)
            {
                const UConnectIt_MapNodeDataAsset* Next = Unlock.Get();
                if (Next && ByData.Contains(Next) && !Reached.Contains(Next))
                {
                    Reached.Add(Next);
                    Queue.Add(Next);
                }
            }
        }
        for (const TPair<const UConnectIt_MapNodeDataAsset*, const UConnectIt_MapNodeComponent*>& Pair : ByData)
        {
            if (!Reached.Contains(Pair.Key))
            {
                Issues.Warnings.Add(FString::Printf(TEXT("%s (on %s) can't be reached from the Start -- no node's "
                    "Unlocks leads to it."), *Pair.Key->GetName(), *OwnerName(Pair.Value)));
            }
        }
    }
    return Issues;
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

    ApplyReveal(RunRevealWalk(StartNode ? StartNode->NodeData.Get() : nullptr, MapNodes, Progress,
        UConnectIt_ActionShardTableDataAsset::Get(), bRevealAllNodes));

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
    BindHover(Node, true);

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
    BindHover(Node, false);
    if (HoveredNode == Node) HoveredNode = nullptr;
    if (SelectedNode == Node) SelectedNode = nullptr;

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

void UConnectIt_AdventureHubComponent::ApplyReveal(const FConnectItRevealWalkResult& Result)
{
    RevealResult = Result;
    bWalked = true;
    for (const TPair<TObjectPtr<UConnectIt_MapNodeDataAsset>, TObjectPtr<UConnectIt_MapNodeComponent>>& Pair : NodeToComponent)
    {
        if (IsValid(Pair.Value)) Pair.Value->SetState(RevealResult.GetState(Pair.Key));
    }
    RebuildLinks();
}

void UConnectIt_AdventureHubComponent::RebuildLinks()
{
    for (AConnectIt_HubLinkActor* Link : LinkActors)
    {
        if (IsValid(Link)) Link->Destroy();
    }
    LinkActors.Reset();

    UWorld* World = GetWorld();
    if (!World) return;

    const TSubclassOf<AConnectIt_HubLinkActor> LinkClass =
        LinkActorClass ? LinkActorClass : TSubclassOf<AConnectIt_HubLinkActor>(AConnectIt_HubLinkActor::StaticClass());
    for (const FConnectItRevealedLink& Link : RevealResult.Links)
    {
        AActor* From = GetActorForNode(Link.From);
        AActor* To = GetActorForNode(Link.To);
        if (!From || !To) continue;

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Params.ObjectFlags |= RF_Transient;
        if (AConnectIt_HubLinkActor* LinkActor = World->SpawnActor<AConnectIt_HubLinkActor>(
            LinkClass, From->GetActorTransform(), Params))
        {
            LinkActor->SetEnds(From, To);
            LinkActors.Add(LinkActor);
        }
    }
}

TArray<AConnectIt_HubLinkActor*> UConnectIt_AdventureHubComponent::GetLinkActors() const
{
    TArray<AConnectIt_HubLinkActor*> Found;
    for (AConnectIt_HubLinkActor* Link : LinkActors)
    {
        if (IsValid(Link)) Found.Add(Link);
    }
    return Found;
}

void UConnectIt_AdventureHubComponent::BindHover(UConnectIt_MapNodeComponent* Node, bool bBind)
{
    UGridHoverComponent* Hover = bBind ? Node->GetOrCreateHover()
        : (Node->GetOwner() ? Node->GetOwner()->FindComponentByClass<UGridHoverComponent>() : nullptr);
    if (!Hover) return;

    if (bBind)
    {
        Hover->OnHoverBegin.AddUniqueDynamic(this, &UConnectIt_AdventureHubComponent::HandleHoverBegin);
        Hover->OnHoverEnd.AddUniqueDynamic(this, &UConnectIt_AdventureHubComponent::HandleHoverEnd);
        Hover->OnClicked.AddUniqueDynamic(this, &UConnectIt_AdventureHubComponent::HandleHoverClicked);
    }
    else
    {
        Hover->OnHoverBegin.RemoveDynamic(this, &UConnectIt_AdventureHubComponent::HandleHoverBegin);
        Hover->OnHoverEnd.RemoveDynamic(this, &UConnectIt_AdventureHubComponent::HandleHoverEnd);
        Hover->OnClicked.RemoveDynamic(this, &UConnectIt_AdventureHubComponent::HandleHoverClicked);
    }
}

UConnectIt_MapNodeComponent* UConnectIt_AdventureHubComponent::NodeForHover(const UGridHoverComponent* Hover) const
{
    const UConnectIt_MapNodeDataAsset* NodeData = Hover ? GetNodeForActor(Hover->GetOwner()) : nullptr;
    UConnectIt_MapNodeComponent* Node = GetNodeComponent(NodeData);
    // A hidden node can't be hovered (no collision), but never react to one
    return Node && Node->GetState() != EConnectItNodeState::Hidden ? Node : nullptr;
}

void UConnectIt_AdventureHubComponent::HandleHoverBegin(UGridHoverComponent* Hover)
{
    UConnectIt_MapNodeComponent* Node = NodeForHover(Hover);
    if (!Node) return;

    HoveredNode = Node;
    if (UTransformLerpComponent* Lerp = Node->GetOwner()->FindComponentByClass<UTransformLerpComponent>())
    {
        Lerp->PlayForward();
    }
    OnNodeHovered.Broadcast(Node);
}

void UConnectIt_AdventureHubComponent::HandleHoverEnd(UGridHoverComponent* Hover)
{
    UConnectIt_MapNodeComponent* Node = NodeForHover(Hover);
    if (!Node) return;

    if (HoveredNode == Node) HoveredNode = nullptr;
    if (UTransformLerpComponent* Lerp = Node->GetOwner()->FindComponentByClass<UTransformLerpComponent>())
    {
        Lerp->PlayReverse();
    }
    OnNodeUnhovered.Broadcast(Node);
}

void UConnectIt_AdventureHubComponent::HandleHoverClicked(UGridHoverComponent* Hover)
{
    SelectNode(NodeForHover(Hover));
}

void UConnectIt_AdventureHubComponent::SelectNode(UConnectIt_MapNodeComponent* Node)
{
    if (!IsValid(Node) || Node->GetState() == EConnectItNodeState::Hidden) return;

    SelectedNode = Node;
    FocusOnNode(Node);
    OnNodeSelected.Broadcast(Node);
}

void UConnectIt_AdventureHubComponent::FocusOnNode_Implementation(UConnectIt_MapNodeComponent* Node)
{
    const AActor* NodeActor = Node ? Node->GetOwner() : nullptr;
    const APlayerController* Player = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    APawn* View = Player ? Player->GetPawn() : nullptr;
    if (!NodeActor || !View) return;

    const FVector Target = NodeActor->GetActorLocation();
    View->SetActorLocation(FVector(Target.X, Target.Y, View->GetActorLocation().Z));
}

FBox UConnectIt_AdventureHubComponent::GetRevealedBounds() const
{
    FBox Bounds(ForceInit);
    for (const TPair<TObjectPtr<UConnectIt_MapNodeDataAsset>, EConnectItNodeState>& Revealed : RevealResult.States)
    {
        if (const AActor* Actor = GetActorForNode(Revealed.Key))
        {
            Bounds += Actor->GetActorLocation();
        }
    }
    return Bounds.IsValid ? Bounds.ExpandBy(FramingMargin) : Bounds;
}

void UConnectIt_AdventureHubComponent::FocusLastPlayed()
{
    const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const UConnectIt_AdventureSubsystem* Adventure = GameInstance ? GameInstance->GetSubsystem<UConnectIt_AdventureSubsystem>() : nullptr;
    const FGuid LastPlayed = Adventure ? Adventure->GetProgress().LastPlayedNode : FGuid();

    UConnectIt_MapNodeComponent* Focus = nullptr;
    for (const TPair<TObjectPtr<UConnectIt_MapNodeDataAsset>, TObjectPtr<UConnectIt_MapNodeComponent>>& Pair : NodeToComponent)
    {
        if (LastPlayed.IsValid() && Pair.Key->NodeId == LastPlayed) Focus = Pair.Value;
    }
    if (!Focus || Focus->GetState() == EConnectItNodeState::Hidden) Focus = StartNode;
    if (!Focus) return;

    for (const TPair<TObjectPtr<UConnectIt_MapNodeDataAsset>, TObjectPtr<UConnectIt_MapNodeComponent>>& Pair : NodeToComponent)
    {
        if (IsValid(Pair.Value)) Pair.Value->SetFocused(Pair.Value == Focus);
    }
    FocusOnNode(Focus);
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
