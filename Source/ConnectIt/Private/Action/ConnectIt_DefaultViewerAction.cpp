// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ConnectIt_DefaultViewerAction.h"
#include "Board/ConnectIt_BoardManager.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"


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
    // Pass this as WorldContextObject
    // GetWorld() resolves via outer chain from the component this lives on
    UConnectIt_BoardStateComponent* BSC =
        UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);

    if (!IsValid(BSC)) return;

    BSC->OnBoardStateChanged.AddDynamic(
        this,
        &UConnectIt_DefaultViewerAction::HandleBoardStateChanged);
}

void UConnectIt_DefaultViewerAction::UnbindFromBoardState()
{
    UConnectIt_BoardStateComponent* BSC =
        UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);

    if (!IsValid(BSC)) return;

    BSC->OnBoardStateChanged.RemoveDynamic(
        this,
        &UConnectIt_DefaultViewerAction::HandleBoardStateChanged);
}

void UConnectIt_DefaultViewerAction::HandleBoardStateChanged()
{
    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_DefaultViewerAction: Board state changed "
             "while observing"));
}