// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedMechanicsStructs.h"
#include "Components/ActorComponent.h"
#include "TurnBasedParticipantComponent.generated.h"

class UTurnBasedParticipantManagerComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnNotification, const FTurnNotification&, Notification);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnyParticipantTurnStarted, bool, bIsMyTurn);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnNotification_Native, const FTurnNotification&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAnyParticipantTurnStarted_Native, bool);

UCLASS(ClassGroup=(TurnBased), meta=(BlueprintSpawnableComponent))
class UNREALTURNBASEDMECHANICS_API UTurnBasedParticipantComponent
    : public UActorComponent
{
    GENERATED_BODY()

public:

    UTurnBasedParticipantComponent();

    // --- State ---

    UPROPERTY(BlueprintReadOnly, Category = "Turn Based|Participant")
    int32 CachedSlotIndex = -1;

    UPROPERTY(BlueprintReadOnly, Category = "Turn Based|Participant")
    EParticipantType ParticipantType = EParticipantType::Human;

    UFUNCTION(BlueprintPure, Category = "Turn Based|Participant")
    bool IsMyTurn() const { return bIsMyTurn; }

    // --- Server RPCs ---

    UFUNCTION(Server, Reliable, BlueprintCallable,
        Category = "Turn Based|Participant")
    void ServerNotifyReady();

    UFUNCTION(Server, Reliable, BlueprintCallable,
        Category = "Turn Based|Participant")
    void ServerSubmitTurnEnd();

    // --- Client RPC ---
    // Single entry point for all turn phase notifications
    // bIsMyTurn updated internally based on phase
    // Subscribers check Notification.Phase to determine relevance
    UFUNCTION(Client, Reliable)
    void ClientReceiveTurnNotification(const FTurnNotification Notification);

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnTurnNotification OnTurnNotificationReceived;

    FOnTurnNotification_Native OnTurnNotificationReceived_Native;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnAnyParticipantTurnStarted OnAnyParticipantTurnStarted;

    FOnAnyParticipantTurnStarted_Native OnAnyParticipantTurnStarted_Native;

    // --- Called by manager ---

    void NotifyAnyParticipantTurnStarted(bool bIsThisMyTurn);

protected:

    virtual void BeginPlay() override;

private:

    bool bIsMyTurn = false;

    // Phases that set bIsMyTurn true
    static bool IsMyTurnStartingPhase(ETurnPhase Phase);

    // Phases that set bIsMyTurn false
    static bool IsMyTurnEndingPhase(ETurnPhase Phase);

    UTurnBasedParticipantManagerComponent* GetParticipantManager() const;
};