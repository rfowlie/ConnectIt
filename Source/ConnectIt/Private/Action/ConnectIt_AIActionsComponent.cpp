// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ConnectIt_AIActionsComponent.h"
#include "Framework/Controller/ConnectIt_AIController.h"


void UConnectIt_AIActionsComponent::OnTurnStarted_Implementation(
    const FTurnStartContext& Context)
{
    Super::OnTurnStarted_Implementation(Context);

    if (AConnectIt_AIController* AIController = Cast<AConnectIt_AIController>(GetOwner()))
    {
        AIController->OnMyTurnStarted();
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIActionsComponent: OnTurnStarted -- owner is not "
                 "AConnectIt_AIController"));
    }
}
