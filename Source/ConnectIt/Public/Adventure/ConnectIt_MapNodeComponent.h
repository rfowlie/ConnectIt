// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Adventure/ConnectIt_AdventureProgress.h"
#include "ConnectIt_MapNodeComponent.generated.h"

class UConnectIt_MapNodeComponent;
class UConnectIt_MapNodeDataAsset;
class UGridHoverComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectItMapNodeStateChanged, UConnectIt_MapNodeComponent*, Node);


// Makes any actor an Adventure hub map node: an artist dresses the actor, this
// says which node it is (its data asset) and shows its state. The hub
// (UConnectIt_AdventureHubComponent, on the hub game mode) finds every node on
// load and sets their states from the player's progress; a node that begins
// play later adds itself.
//
// Hidden hides the actor (and its collision); any other state shows it.
// Like a grid tile, it sets its actor's root primitive to block the cursor
// trace channel (UGridMechanicsSettings), so the node can be hovered and
// clicked with no collision work.
// Blueprint reacts to OnStateChanged (looks per state, the "you were here"
// focus). Nodes don't react to hover or clicks themselves.
UCLASS(ClassGroup = (Adventure), meta = (BlueprintSpawnableComponent))
class CONNECTIT_API UConnectIt_MapNodeComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UConnectIt_MapNodeComponent();

    // Which node this actor is
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Node")
    TObjectPtr<UConnectIt_MapNodeDataAsset> NodeData;

    // Where the reveal walk starts (one per hub map)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Node")
    bool bIsStart = false;

    // A sub-map's way out: completing it completes the node the sub-map
    // stands for (one per sub-map)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Node")
    bool bIsExit = false;

    UFUNCTION(BlueprintPure, Category = "Map Node")
    EConnectItNodeState GetState() const { return State; }

    // The hub's: the node's state for the player (Hidden hides the actor)
    UFUNCTION(BlueprintCallable, Category = "Map Node")
    void SetState(EConnectItNodeState NewState);

    UFUNCTION(BlueprintPure, Category = "Map Node")
    bool IsFocused() const { return bFocused; }

    // The hub's: this is the node the hub is focused on (the last played)
    UFUNCTION(BlueprintCallable, Category = "Map Node")
    void SetFocused(bool bInFocused);

    // State or focus changed
    UPROPERTY(BlueprintAssignable, Category = "Map Node")
    FOnConnectItMapNodeStateChanged OnStateChanged;

    // The actor's grid hover component (category ConnectIt.Adventure.MapNode),
    // made if the actor has none: hover and clicks go to the hub through it
    UGridHoverComponent* GetOrCreateHover();

protected:

    // Applies the state to the actor, then adds itself to the hub if the hub
    // has already found its nodes
    virtual void OnRegister() override;
    virtual void BeginPlay() override;

    // Leaves the hub
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

    void ApplyVisibility() const;

    UPROPERTY(VisibleInstanceOnly, Category = "Map Node")
    EConnectItNodeState State = EConnectItNodeState::Hidden;

    bool bFocused = false;
};
