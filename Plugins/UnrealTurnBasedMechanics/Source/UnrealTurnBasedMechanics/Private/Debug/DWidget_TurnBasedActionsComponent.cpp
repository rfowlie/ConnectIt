// Fill out your copyright notice in the Description page of Project Settings.

#include "Debug/DWidget_TurnBasedActionsComponent.h"
#include "Action/TurnBasedAction.h"
#include "Action/TurnBasedActionBase.h"
#include "Action/TurnBasedActionsComponent.h"
#include "TurnBasedMechanicsStructs.h"


void UDWidget_TurnBasedActionsComponent::BindDelegates()
{
    APlayerController* OwningPC = GetOwningPlayer();
    if (!IsValid(OwningPC)) return;

    ResolvedSource = OwningPC->FindComponentByClass<UTurnBasedActionsComponent>();
    if (!IsValid(ResolvedSource)) return;

    ResolvedSource->OnActionPushed.AddDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleActionStackChanged);
    ResolvedSource->OnActionPopped.AddDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleActionStackChanged);
    ResolvedSource->OnActionCompleted.AddDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleActionResolved);
    ResolvedSource->OnActionCancelled.AddDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleActionResolved);
    ResolvedSource->OnBoardChangeRequested.AddDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleBoardChangeRequested);
}

void UDWidget_TurnBasedActionsComponent::UnbindDelegates()
{
    if (!IsValid(ResolvedSource)) return;

    ResolvedSource->OnActionPushed.RemoveDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleActionStackChanged);
    ResolvedSource->OnActionPopped.RemoveDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleActionStackChanged);
    ResolvedSource->OnActionCompleted.RemoveDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleActionResolved);
    ResolvedSource->OnActionCancelled.RemoveDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleActionResolved);
    ResolvedSource->OnBoardChangeRequested.RemoveDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleBoardChangeRequested);
}

void UDWidget_TurnBasedActionsComponent::RefreshFields()
{
    bSourceValid = IsValid(ResolvedSource);
    if (!bSourceValid) return;

    if (UTurnBasedActionBase* Top = ResolvedSource->GetTopAction())
    {
        CachedTopActionTag = Top->ActionTag;
    }
    else
    {
        CachedTopActionTag = FGameplayTag();
    }

    if (UTurnBasedActionBase* Root = ResolvedSource->GetRootAction())
    {
        CachedRootActionTag = Root->ActionTag;
    }
    else
    {
        CachedRootActionTag = FGameplayTag();
    }

    CachedStackDepth = ResolvedSource->GetStackDepth();
    bCachedAwaitingRequestConfirmation = ResolvedSource->IsAwaitingRequestConfirmation();
}

void UDWidget_TurnBasedActionsComponent::HandleActionStackChanged(UTurnBasedActionBase* Action)
{
    RefreshAll();
}

void UDWidget_TurnBasedActionsComponent::HandleActionResolved(UTurnBasedAction* Action)
{
    RefreshAll();
}

void UDWidget_TurnBasedActionsComponent::HandleBoardChangeRequested(const FTurnActionRequest& Request)
{
    RefreshAll();
}
