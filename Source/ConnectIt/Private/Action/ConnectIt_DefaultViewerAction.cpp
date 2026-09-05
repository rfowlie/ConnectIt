// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ConnectIt_DefaultViewerAction.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"


void UConnectIt_DefaultViewerAction::PostInitialiseAction_Implementation()
{
    CachedBoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(OwningController);

    if (!IsValid(CachedBoardState))
    {
        UE_LOG(LogTemp, Error,
            TEXT("DefaultViewerAction: PostInitialiseAction -- "
                 "no UConnectIt_BoardStateComponent found"));
    }
}

void UConnectIt_DefaultViewerAction::Activate_Internal_Implementation()
{
    BindToBoardState();

    if (APlayerController* PC = Cast<APlayerController>(OwningController))
    {
        PC->SetShowMouseCursor(true);
    }
}

void UConnectIt_DefaultViewerAction::Deactivate_Internal_Implementation()
{
    UnbindFromBoardState();
}

void UConnectIt_DefaultViewerAction::BindToBoardState()
{
    if (!IsValid(CachedBoardState)) return;

    CachedBoardState->OnBoardStateChanged.AddUniqueDynamic(
        this,
        &UConnectIt_DefaultViewerAction::HandleBoardStateChanged);
}

void UConnectIt_DefaultViewerAction::UnbindFromBoardState()
{
    if (!IsValid(CachedBoardState)) return;

    CachedBoardState->OnBoardStateChanged.RemoveDynamic(
        this,
        &UConnectIt_DefaultViewerAction::HandleBoardStateChanged);
}

void UConnectIt_DefaultViewerAction::HandleBoardStateChanged()
{
    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_DefaultViewerAction: Board state changed "
             "while observing"));
}