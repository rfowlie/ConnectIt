// Fill out your copyright notice in the Description page of Project Settings.

#include "Adventure/ConnectIt_MapNodeComponent.h"
#include "Adventure/ConnectIt_AdventureHubComponent.h"
#include "ConnectIt_GameplayTags.h"
#include "Hover/GridHoverComponent.h"
#include "GridMechanicsSettings.h"
#include "GameFramework/Actor.h"


UConnectIt_MapNodeComponent::UConnectIt_MapNodeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UConnectIt_MapNodeComponent::SetState(EConnectItNodeState NewState)
{
    if (State == NewState) return;
    State = NewState;
    ApplyVisibility();
    OnStateChanged.Broadcast(this);
}

void UConnectIt_MapNodeComponent::SetFocused(bool bInFocused)
{
    if (bFocused == bInFocused) return;
    bFocused = bInFocused;
    OnStateChanged.Broadcast(this);
}

void UConnectIt_MapNodeComponent::OnRegister()
{
    Super::OnRegister();

    // As a grid tile does (AGridTileBase::OnConstruction): the cursor can hit it
    UGridMechanicsSettings::SetRootCursorResponse(GetOwner(), ECR_Block);
}

void UConnectIt_MapNodeComponent::BeginPlay()
{
    Super::BeginPlay();
    ApplyVisibility();

    // Found already by the hub's discovery, or arriving after it (a late
    // spawn, a streamed level): either way add-or-update
    if (UConnectIt_AdventureHubComponent* Hub = UConnectIt_AdventureHubComponent::GetAdventureHub(this))
    {
        if (Hub->HasDiscovered())
        {
            Hub->AddNode(this);
        }
    }
}

void UConnectIt_MapNodeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UConnectIt_AdventureHubComponent* Hub = UConnectIt_AdventureHubComponent::GetAdventureHub(this))
    {
        Hub->RemoveNode(this);
    }
    Super::EndPlay(EndPlayReason);
}

UGridHoverComponent* UConnectIt_MapNodeComponent::GetOrCreateHover()
{
    AActor* Owner = GetOwner();
    if (!Owner) return nullptr;

    UGridHoverComponent* Hover = Owner->FindComponentByClass<UGridHoverComponent>();
    if (!Hover)
    {
        Hover = NewObject<UGridHoverComponent>(Owner, TEXT("MapNodeHover"));
        Owner->AddInstanceComponent(Hover);
        Hover->RegisterComponent();
    }
    Hover->HoverCategory = ConnectIt_Adventure_MapNode;
    return Hover;
}

void UConnectIt_MapNodeComponent::ApplyVisibility() const
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    const bool bHidden = State == EConnectItNodeState::Hidden;
    Owner->SetActorHiddenInGame(bHidden);
    Owner->SetActorEnableCollision(!bHidden);
}
