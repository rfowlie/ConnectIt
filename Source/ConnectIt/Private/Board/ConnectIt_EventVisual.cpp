// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/ConnectIt_EventVisual.h"
#include "Board/ConnectIt_BoardVisualizer.h"
#include "Board/ConnectIt_VisualHandle.h"


AConnectIt_EventVisual::AConnectIt_EventVisual()
{
    PrimaryActorTick.bCanEverTick = false;
    // Local to each machine, like the visualizer
    bReplicates = false;
}

FGameplayTag AConnectIt_EventVisual::GetEventTagOf(TSubclassOf<AConnectIt_EventVisual> VisualClass)
{
    return VisualClass ? VisualClass->GetDefaultObject<AConnectIt_EventVisual>()->GetEventTag() : FGameplayTag();
}

void AConnectIt_EventVisual::PlayEvent(AConnectIt_BoardVisualizer* InVisualizer, const FInstancedStruct& Payload,
    UConnectIt_VisualHandle* Handle)
{
    Visualizer = InVisualizer;
    ActivePayload = Payload;
    ActiveHandle = Handle;
    Play(Payload, Handle);
}

void AConnectIt_EventVisual::PlayDefault()
{
    if (IsValid(Visualizer) && ActivePayload.IsValid())
    {
        Visualizer->PlayDefaultVisuals(ActivePayload, ActiveHandle);
    }
}
