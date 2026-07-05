// Fill out your copyright notice in the Description page of Project Settings.

#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "TurnBasedStrategyStructs.h"


UTurnBasedParticipantComponent::UTurnBasedParticipantComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UTurnBasedParticipantComponent::BeginPlay()
{
    Super::BeginPlay();
}

// --- Server RPCs ---

void UTurnBasedParticipantComponent::ServerNotifyReady_Implementation()
{
    if (ATurnBasedPlayerState* PS = GetOwner<AController>()->GetPlayerState<ATurnBasedPlayerState>())
    {
        PS->Server_SetReady(true);

        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedParticipantComponent: %s confirmed ready"),
            *GetOwner()->GetName());
    }
}

void UTurnBasedParticipantComponent::ServerSubmitTurnEnd_Implementation()
{
    if (!bIsMyTurn)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedParticipantComponent: %s submitted turn end but it is not their turn"),
            *GetOwner()->GetName());
        return;
    }

    // Manager picks this up via PlayerState or direct reference
    // Actual processing happens in UTurnBasedParticipantManagerComponent
    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedParticipantComponent: %s submitted turn end"),
        *GetOwner()->GetName());
}

// --- Client RPCs ---

void UTurnBasedParticipantComponent::ClientNotifyTurnStarted_Implementation(
    FTurnNotification Notification)
{
    bIsMyTurn = true;
    HandleTurnStarted(Notification);
    OnTurnStarted.Broadcast(Notification);
    OnTurnStarted_Native.Broadcast(Notification);
}

void UTurnBasedParticipantComponent::ClientNotifyTurnEnded_Implementation(
    FTurnNotification Notification)
{
    bIsMyTurn = false;
    HandleTurnEnded(Notification);
    OnTurnEnded.Broadcast(Notification);
    OnTurnEnded_Native.Broadcast(Notification);
}

void UTurnBasedParticipantComponent::ClientNotifyTurnPaused_Implementation(
    FTurnNotification Notification)
{
    HandleTurnPaused(Notification);
    OnTurnPaused.Broadcast(Notification);
    OnTurnPaused_Native.Broadcast(Notification);
}

void UTurnBasedParticipantComponent::ClientNotifyTurnResumed_Implementation(
    FTurnNotification Notification)
{
    HandleTurnResumed(Notification);
    OnTurnResumed.Broadcast(Notification);
    OnTurnResumed_Native.Broadcast(Notification);
}

void UTurnBasedParticipantComponent::ClientNotifyTurnTimedOut_Implementation(
    FTurnNotification Notification)
{
    bIsMyTurn = false;
    HandleTurnTimedOut(Notification);
    OnTurnTimedOut.Broadcast(Notification);
    OnTurnTimedOut_Native.Broadcast(Notification);
}

void UTurnBasedParticipantComponent::ClientNotifyTurnSkipped_Implementation(
    FTurnNotification Notification)
{
    bIsMyTurn = false;
    HandleTurnSkipped(Notification);
    OnTurnSkipped.Broadcast(Notification);
    OnTurnSkipped_Native.Broadcast(Notification);
}

void UTurnBasedParticipantComponent::ClientNotifyForfeited_Implementation(
    FTurnNotification Notification)
{
    bIsMyTurn = false;
    HandleForfeited(Notification);
    OnForfeited.Broadcast(Notification);
    OnForfeited_Native.Broadcast(Notification);
}

// --- Virtual handlers ---

void UTurnBasedParticipantComponent::HandleTurnStarted(
    const FTurnNotification& Notification)
{
    // Base does nothing — subclasses override
}

void UTurnBasedParticipantComponent::HandleTurnEnded(
    const FTurnNotification& Notification)
{
}

void UTurnBasedParticipantComponent::HandleTurnPaused(
    const FTurnNotification& Notification)
{
}

void UTurnBasedParticipantComponent::HandleTurnResumed(
    const FTurnNotification& Notification)
{
}

void UTurnBasedParticipantComponent::HandleTurnTimedOut(
    const FTurnNotification& Notification)
{
}

void UTurnBasedParticipantComponent::HandleTurnSkipped(
    const FTurnNotification& Notification)
{
}

void UTurnBasedParticipantComponent::HandleForfeited(
    const FTurnNotification& Notification)
{
}
