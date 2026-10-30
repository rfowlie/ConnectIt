// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ConnectIt_AdventureHubComponent.generated.h"

class UConnectIt_MapNodeComponent;
class UConnectIt_MapNodeDataAsset;


// The hub map's system, on its game mode (AConnectIt_GameMode_AdventureHub),
// so it exists only in hub maps (server / standalone: Adventure is
// single-player). Keeps which actor is which node, both ways.
//
// Discovery is pulled: when it begins play it finds every map node component
// in the world (load order doesn't matter). A node that begins play later adds
// itself (add-or-update); one leaving play removes itself. The reveal walk
// that sets the nodes' states runs after discovery.
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

protected:

    virtual void BeginPlay() override;

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

    bool bDiscovered = false;
};
