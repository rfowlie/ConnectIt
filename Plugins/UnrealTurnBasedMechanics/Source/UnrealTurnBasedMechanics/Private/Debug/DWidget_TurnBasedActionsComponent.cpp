// Fill out your copyright notice in the Description page of Project Settings.

#include "Debug/DWidget_TurnBasedActionsComponent.h"
#include "Action/TurnBasedActionsComponent.h"


void UDWidget_TurnBasedActionsComponent::BindDelegates()
{
    APlayerController* OwningPC = GetOwningPlayer();
    if (!IsValid(OwningPC)) return;

    ResolvedSource = OwningPC->FindComponentByClass<UTurnBasedActionsComponent>();
    if (!IsValid(ResolvedSource))
    {
        UE_LOG(LogDWidget, Error,
            TEXT("UDWidget_ConnectIt_GameState::BindDelegates - Source Invalid"));
        return;
    }

    ResolvedSource->OnActionPushedSafe.AddDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleActionStackChangedSafe);
    ResolvedSource->OnActionPoppedSafe.AddDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleActionStackChangedSafe);
    ResolvedSource->OnActionCompletedSafe.AddDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleActionStackChangedSafe);
    ResolvedSource->OnActionCancelledSafe.AddDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleActionStackChangedSafe);
    ResolvedSource->OnBoardChangeRequested.AddDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleBoardChangeRequested);

    PushCurrentInfo();
}

void UDWidget_TurnBasedActionsComponent::UnbindDelegates()
{
    if (!IsValid(ResolvedSource))
    {
        UE_LOG(LogDWidget, Error,
            TEXT("UDWidget_TurnBasedActionsComponent::UnbindDelegates - Source Invalid"));
        return;
    }

    ResolvedSource->OnActionPushedSafe.RemoveDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleActionStackChangedSafe);
    ResolvedSource->OnActionPoppedSafe.RemoveDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleActionStackChangedSafe);
    ResolvedSource->OnActionCompletedSafe.RemoveDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleActionStackChangedSafe);
    ResolvedSource->OnActionCancelledSafe.RemoveDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleActionStackChangedSafe);
    ResolvedSource->OnBoardChangeRequested.RemoveDynamic(
        this, &UDWidget_TurnBasedActionsComponent::HandleBoardChangeRequested);
}

void UDWidget_TurnBasedActionsComponent::HandleActionStackChangedSafe(const FTurnActionSnapshot& Snapshot)
{
    PushCurrentInfo();
}

void UDWidget_TurnBasedActionsComponent::HandleBoardChangeRequested(const FTurnActionRequest& Request)
{
    PushCurrentInfo();
}

void UDWidget_TurnBasedActionsComponent::PushCurrentInfo()
{
    if (!IsValid(ResolvedSource))
    {
        UE_LOG(LogDWidget, Error,
            TEXT("UDWidget_TurnBasedActionsComponent::PushCurrentInfo - Source Invalid"));
        return;
    }

    const FTurnBasedActionsComponentInfo Info = ResolvedSource->GetInfo();
    OnTopActionUpdated(Info.TopActionTag);
    OnRootActionUpdated(Info.RootActionTag);
    OnStackDepthUpdated(Info.StackDepth);
    OnAwaitingRequestConfirmationUpdated(Info.bAwaitingRequestConfirmation);
}
