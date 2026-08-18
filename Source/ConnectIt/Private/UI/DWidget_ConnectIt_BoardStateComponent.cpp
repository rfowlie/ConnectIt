// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/DWidget_ConnectIt_BoardStateComponent.h"

#include "Board/ConnectIt_BoardManager.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Framework/Subsystem/ConnectIt_BoardManagerSubsystem.h"

void UDWidget_ConnectIt_BoardStateComponent::BindDelegates()
{
    ResolvedBoardManagerSubsystem = GetWorld()
        ? GetWorld()->GetSubsystem<UConnectIt_BoardManagerSubsystem>()
        : nullptr;

    if (!IsValid(ResolvedBoardManagerSubsystem)) return;

    if (AConnectIt_BoardManager* BoardManager = ResolvedBoardManagerSubsystem->GetBoardManager())
    {
        BindBoardManager(BoardManager);
    }
    else
    {
        ResolvedBoardManagerSubsystem->OnBoardManagerReady.AddDynamic(
            this, &UDWidget_ConnectIt_BoardStateComponent::HandleBoardManagerReady);
    }
}

void UDWidget_ConnectIt_BoardStateComponent::UnbindDelegates()
{
    if (IsValid(ResolvedBoardManagerSubsystem))
    {
        ResolvedBoardManagerSubsystem->OnBoardManagerReady.RemoveDynamic(
            this, &UDWidget_ConnectIt_BoardStateComponent::HandleBoardManagerReady);
    }

    if (IsValid(ResolvedSource))
    {
        ResolvedSource->OnBoardStateChanged.RemoveDynamic(
            this, &UDWidget_ConnectIt_BoardStateComponent::HandleBoardStateChanged);
    }
}

void UDWidget_ConnectIt_BoardStateComponent::BindBoardManager(AConnectIt_BoardManager* InBoardManager)
{
    if (!IsValid(InBoardManager)) return;

    ResolvedSource = InBoardManager->GetBoardStateComponent();

    if (IsValid(ResolvedSource))
    {
        ResolvedSource->OnBoardStateChanged.AddDynamic(
            this, &UDWidget_ConnectIt_BoardStateComponent::HandleBoardStateChanged);
    }
}

void UDWidget_ConnectIt_BoardStateComponent::RefreshFields()
{
    bSourceValid = IsValid(ResolvedSource);
    if (!bSourceValid) return;

    CachedCurrentState = ResolvedSource->GetCurrentState();
    CachedLastChangeEvent = ResolvedSource->GetChangeEvent();
}

void UDWidget_ConnectIt_BoardStateComponent::HandleBoardManagerReady(AConnectIt_BoardManager* InBoardManager)
{
    BindBoardManager(InBoardManager);
    RefreshAll();
}

void UDWidget_ConnectIt_BoardStateComponent::HandleBoardStateChanged()
{
    RefreshAll();
}
