// Fill out your copyright notice in the Description page of Project Settings.

#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "UnrealTurnBasedMechanics.h"
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
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedParticipantComponent: %s submitted turn end "
                 "but bIsMyTurn is false — ignored"),
            *GetOwner()->GetName());
        return;
    }

    UTurnBasedParticipantManagerComponent* Manager = GetParticipantManager();
    if (!IsValid(Manager)) return;
    Manager->NotifyTurnEndSubmitted(GetOwner<AController>());
}

// --- Client RPC ---

void UTurnBasedParticipantComponent::ClientReceiveTurnNotification_Implementation(
    FTurnNotification Notification)
{
    if (IsMyTurnStartingPhase(Notification.Phase))
    {
        bIsMyTurn = true;
    }
    else if (IsMyTurnEndingPhase(Notification.Phase))
    {
        bIsMyTurn = false;
    }

    OnTurnNotificationReceived.Broadcast(Notification);
    OnTurnNotificationReceived_Native.Broadcast(Notification);
}

// --- Manager Callback ---

void UTurnBasedParticipantComponent::ReceiveOpponentTurnStarted(
    int32 ActiveParticipantSlotIndex)
{
    // Not my turn
    bIsMyTurn = false;

    OnOpponentTurnStarted.Broadcast(ActiveParticipantSlotIndex);
    OnOpponentTurnStarted_Native.Broadcast(ActiveParticipantSlotIndex);
}

// --- Static Helpers ---

bool UTurnBasedParticipantComponent::IsMyTurnStartingPhase(ETurnPhase Phase)
{
    return Phase == ETurnPhase::TurnStart
        || Phase == ETurnPhase::TurnActive;
}

bool UTurnBasedParticipantComponent::IsMyTurnEndingPhase(ETurnPhase Phase)
{
    return Phase == ETurnPhase::TurnEnd
        || Phase == ETurnPhase::TurnTimeout
        || Phase == ETurnPhase::TurnSkipped
        || Phase == ETurnPhase::GameOver;
}

// --- Helper ---

UTurnBasedParticipantManagerComponent*
UTurnBasedParticipantComponent::GetParticipantManager() const
{
    UWorld* World = GetWorld();
    if (!IsValid(World)) return nullptr;

    AGameStateBase* GS = World->GetGameState();
    if (!IsValid(GS)) return nullptr;

    return GS->FindComponentByClass<UTurnBasedParticipantManagerComponent>();
}