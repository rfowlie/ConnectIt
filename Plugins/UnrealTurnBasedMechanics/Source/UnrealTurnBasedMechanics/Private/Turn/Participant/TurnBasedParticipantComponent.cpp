// Fill out your copyright notice in the Description page of Project Settings.

#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "TurnBasedMechanicsStructs.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"


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
    UTurnBasedParticipantManagerComponent* Manager = GetParticipantManager();
    if (!IsValid(Manager)) return;

    Manager->NotifyParticipantReady(GetOwner<AController>());
}

void UTurnBasedParticipantComponent::ServerSubmitTurnEnd_Implementation()
{
    if (!bIsMyTurn)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedParticipantComponent: %s submitted turn end "
                 "but bIsMyTurn is false — ignored"),
            *GetOwner()->GetName());
        return;
    }

    UTurnBasedParticipantManagerComponent* Manager = GetParticipantManager();
    if (!IsValid(Manager)) return;

    Manager->NotifyTurnEndSubmitted(GetOwner<AController>());
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

// --- Manager callback ---

void UTurnBasedParticipantComponent::HandleAnyParticipantTurnStarted( bool bIsTurn)
{
    // Fire both Blueprint and Native delegates
    // UTurnBasedActionComponent binds here for cooldown ticking
    OnAnyParticipantTurnStarted.Broadcast(bIsTurn);
    OnAnyParticipantTurnStarted_Native.Broadcast(bIsTurn);
}

// --- Virtual handlers ---

void UTurnBasedParticipantComponent::HandleTurnStarted(
    const FTurnNotification& Notification)
{
    // Base does nothing
    // AI subclass overrides to begin MinMax computation
    // Player subclass may override to enable input
}

void UTurnBasedParticipantComponent::HandleTurnEnded(
    const FTurnNotification& Notification) {}

void UTurnBasedParticipantComponent::HandleTurnPaused(
    const FTurnNotification& Notification) {}

void UTurnBasedParticipantComponent::HandleTurnResumed(
    const FTurnNotification& Notification) {}

void UTurnBasedParticipantComponent::HandleTurnTimedOut(
    const FTurnNotification& Notification) {}

void UTurnBasedParticipantComponent::HandleTurnSkipped(
    const FTurnNotification& Notification) {}

void UTurnBasedParticipantComponent::HandleForfeited(
    const FTurnNotification& Notification) {}

// --- Helper ---

UTurnBasedParticipantManagerComponent* UTurnBasedParticipantComponent::GetParticipantManager() const
{
    const UWorld* World = GetWorld();
    if (!IsValid(World)) return nullptr;

    const AGameStateBase* GS = World->GetGameState();
    if (!IsValid(GS)) return nullptr;

    return GS->FindComponentByClass<UTurnBasedParticipantManagerComponent>();
}