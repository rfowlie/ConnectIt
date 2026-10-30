// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Adventure/ConnectIt_AdventureProgress.h"
#include "ConnectIt_AdventureHubComponent.generated.h"

class UConnectIt_MapNodeComponent;
class UConnectIt_MapNodeDataAsset;
class UConnectIt_ActionShardTableDataAsset;

// One unlock link the walk revealed: From's Unlocks names To, both revealed
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItRevealedLink
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Adventure Hub")
    TObjectPtr<UConnectIt_MapNodeDataAsset> From = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Adventure Hub")
    TObjectPtr<UConnectIt_MapNodeDataAsset> To = nullptr;
};

// What the reveal walk found: each revealed node's state (a node not in it is
// Hidden) and the links between revealed nodes
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItRevealWalkResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Adventure Hub")
    TMap<TObjectPtr<UConnectIt_MapNodeDataAsset>, EConnectItNodeState> States;

    UPROPERTY(BlueprintReadOnly, Category = "Adventure Hub")
    TArray<FConnectItRevealedLink> Links;

    EConnectItNodeState GetState(const UConnectIt_MapNodeDataAsset* Node) const
    {
        const EConnectItNodeState* Found = States.Find(Node);
        return Found ? *Found : EConnectItNodeState::Hidden;
    }
};


// The hub map's system, on its game mode (AConnectIt_GameMode_AdventureHub),
// so it exists only in hub maps (server / standalone: Adventure is
// single-player). Keeps which actor is which node, both ways.
//
// Discovery is pulled: when it begins play it finds every map node component
// in the world (load order doesn't matter). A node that begins play later adds
// itself (add-or-update); one leaving play removes itself.
//
// After discovery the reveal walk (RunRevealWalk) sets every node's state from
// the player's progress (UConnectIt_AdventureSubsystem); it runs again when the
// progress changes (a seal broken). A late node takes its state from the last
// walk. Console: ConnectIt.Adventure.RevealAll 1 / 0 reveals every node.
UCLASS(ClassGroup = (Adventure), meta = (BlueprintSpawnableComponent))
class CONNECTIT_API UConnectIt_AdventureHubComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UConnectIt_AdventureHubComponent();

    // The hub of the map WorldContextObject is in: its game mode's component.
    // Null outside hub maps (and on clients).
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure Hub", meta = (WorldContext = "WorldContextObject"))
    static UConnectIt_AdventureHubComponent* GetAdventureHub(const UObject* WorldContextObject);

    // Finds every map node component in this world (once per map load;
    // called on BeginPlay). A node naming the same data asset as one already
    // found is rejected (logged); the first is kept.
    void DiscoverNodes();

    bool HasDiscovered() const { return bDiscovered; }

    // Adds Node, or updates it if it is already here. Rejected (logged) if it
    // has no data asset, or another node already has its data asset.
    void AddNode(UConnectIt_MapNodeComponent* Node);

    // Forgets Node (both ways)
    void RemoveNode(UConnectIt_MapNodeComponent* Node);

    // --- Queries ---

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure Hub")
    AActor* GetActorForNode(const UConnectIt_MapNodeDataAsset* NodeData) const;

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure Hub")
    UConnectIt_MapNodeDataAsset* GetNodeForActor(const AActor* Actor) const;

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure Hub")
    UConnectIt_MapNodeComponent* GetNodeComponent(const UConnectIt_MapNodeDataAsset* NodeData) const;

    // Where the reveal walk starts (null if no node is flagged Start)
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure Hub")
    UConnectIt_MapNodeComponent* GetStartNode() const { return StartNode; }

    // A sub-map's way out (null if none is flagged Exit)
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure Hub")
    UConnectIt_MapNodeComponent* GetExitNode() const { return ExitNode; }

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure Hub")
    int32 GetNodeCount() const { return NodeToComponent.Num(); }

    // --- The reveal walk ---

    // Which nodes the player sees, and how (no world needed): breadth-first
    // from Start through the Unlocks of completed nodes -- a completed node
    // reveals its unlocks, any other revealed node is the frontier (its
    // unlocks stay hidden). A revealed node's state comes from Progress
    // (FConnectItAdventureProgress::GetNodeState). Only nodes in MapNodes (this
    // map's) are followed: an unlock elsewhere, or an id the save knows but no
    // node has, is skipped. A visited set keeps loops finite. bRevealAll
    // (debug) reveals every node in MapNodes.
    static FConnectItRevealWalkResult RunRevealWalk(const UConnectIt_MapNodeDataAsset* Start,
        const TSet<const UConnectIt_MapNodeDataAsset*>& MapNodes, const FConnectItAdventureProgress& Progress,
        const UConnectIt_ActionShardTableDataAsset* Table, bool bRevealAll = false);

    // Runs the walk with the player's progress and applies it to the nodes
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Adventure Hub")
    void RefreshReveal();

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure Hub")
    const FConnectItRevealWalkResult& GetRevealResult() const { return RevealResult; }

    // Debug (ConnectIt.Adventure.RevealAll): every node revealed
    static bool bRevealAllNodes;

protected:

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

    // Data asset -> its node component (and through it the actor)
    UPROPERTY()
    TMap<TObjectPtr<UConnectIt_MapNodeDataAsset>, TObjectPtr<UConnectIt_MapNodeComponent>> NodeToComponent;

    // Actor -> its data asset
    UPROPERTY()
    TMap<TObjectPtr<AActor>, TObjectPtr<UConnectIt_MapNodeDataAsset>> ActorToNode;

    UPROPERTY()
    TObjectPtr<UConnectIt_MapNodeComponent> StartNode;

    UPROPERTY()
    TObjectPtr<UConnectIt_MapNodeComponent> ExitNode;

    // The last walk (a late node takes its state from it)
    UPROPERTY()
    FConnectItRevealWalkResult RevealResult;

    bool bDiscovered = false;
    bool bWalked = false;

    UFUNCTION()
    void HandleProgressChanged();
};
