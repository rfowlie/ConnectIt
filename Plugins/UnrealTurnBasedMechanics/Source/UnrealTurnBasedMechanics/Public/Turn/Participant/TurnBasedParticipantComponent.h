// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedMechanicsStructs.h"
#include "TurnBasedMechanicsDelegates.h"
#include "TurnBasedParticipantComponent.generated.h"

class UTurnBasedParticipantManagerComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnParticipantNotification, const FTurnNotification&, Notification);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOpponentParticipantTurnStarted, int32, ActiveParticipantSlotIndex);

UCLASS(ClassGroup=(TurnBased), meta=(BlueprintSpawnableComponent))
class UNREALTURNBASEDMECHANICS_API UTurnBasedParticipantComponent : public UActorComponent
{
    GENERATED_BODY()

    friend class UTurnBasedParticipantManagerComponent;

public:

    UTurnBasedParticipantComponent();

    // --- State ---

    UFUNCTION(BlueprintPure, Category = "Turn Based|Participant")
    int32 GetActiveParticipantSlotIndex() const { return CachedSlotIndex; }
    
    UFUNCTION(BlueprintPure, Category = "Turn Based|Participant")
    EParticipantType GetParticipantType() const { return ParticipantType; }
    
    UFUNCTION(BlueprintPure, Category = "Turn Based|Participant")
    bool IsMyTurn() const { return bIsMyTurn; }

    // --- Server RPCs ---

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Turn Based|Participant")
    void ServerNotifyReady();

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Turn Based|Participant")
    void ServerSubmitTurnEnd();

    // --- Client RPC ---
    // Single entry point for all turn phase notifications
    // to the active participant
    UFUNCTION(Client, Reliable)
    void ClientReceiveTurnNotification(FTurnNotification Notification);

    // --- Called by manager ---

    // Called on all non-active participants when a turn starts
    // Not an RPC -- fires local delegate only
    // Manager calls this server side, replication not needed here
    // since participant components exist on each machine independently
    UFUNCTION(Client, Reliable)
    void ClientReceiveOpponentTurnStarted(int32 ActiveParticipantSlotIndex);

    // --- Delegates ---

    // Fired on this participant when it is their turn
    // Carries full notification with phase, turn number, duration
    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnTurnParticipantNotification OnTurnNotificationReceived;

    FOnTurnNotification_Native OnTurnNotificationReceived_Native;

    // Fired on this participant when it is someone else's turn
    // Carries slot index of the now-active participant
    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnOpponentParticipantTurnStarted OnOpponentTurnStarted;

    FOnOpponentTurnStarted_Native OnOpponentTurnStarted_Native;

protected:

    virtual void BeginPlay() override;

private:

    UPROPERTY()
    int32 CachedSlotIndex = -1;
    EParticipantType ParticipantType = EParticipantType::Human;

    // Client-side only -- set by ClientReceiveTurnNotification
    // Never valid on server -- use manager ActiveParticipantIndex instead
    bool bIsMyTurn = false;

    static bool IsMyTurnStartingPhase(ETurnPhase Phase);
    static bool IsMyTurnEndingPhase(ETurnPhase Phase);

    UTurnBasedParticipantManagerComponent* GetParticipantManager() const;
};