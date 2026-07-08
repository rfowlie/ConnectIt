// Fill out your copyright notice in the Description page of Project Settings.


#pragma once

#include "CoreMinimal.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedMechanicsStructs.h"
#include "Components/ActorComponent.h"
#include "TurnBasedParticipantComponent.generated.h"

class UTurnBasedParticipantManagerComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnNotificationDelegate, const FTurnNotification&, Notification);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnyParticipantTurnStarted, bool, bIsMyTurn);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnNotificationDelegate_Native, const FTurnNotification&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAnyParticipantTurnStarted_Native, bool);

UCLASS(ClassGroup=(TurnBased), meta=(BlueprintSpawnableComponent))
class UNREALTURNBASEDMECHANICS_API UTurnBasedParticipantComponent
    : public UActorComponent
{
    GENERATED_BODY()

public:

    UTurnBasedParticipantComponent();

    // --- State ---

    // Set by manager on registration -- stable for lifetime of game
    // Doubles as FactionID in 1v1 games
    UPROPERTY(BlueprintReadOnly, Category = "Turn Based|Participant")
    int32 CachedSlotIndex = -1;

    UPROPERTY(BlueprintReadOnly, Category = "Turn Based|Participant")
    EParticipantType ParticipantType = EParticipantType::Human;

    UFUNCTION(BlueprintPure, Category = "Turn Based|Participant")
    bool IsMyTurn() const { return bIsMyTurn; }

    // --- Server RPCs --- Client -> Server ---

    // Human participant confirms they are ready to start
    UFUNCTION(Server, Reliable, BlueprintCallable,
        Category = "Turn Based|Participant")
    void ServerNotifyReady();

    // Human participant submits turn end
    // Validated server side -- must be active participant
    UFUNCTION(Server, Reliable, BlueprintCallable,
        Category = "Turn Based|Participant")
    void ServerSubmitTurnEnd();

    // --- Client RPCs --- Server -> specific client ---

    UFUNCTION(Client, Reliable)
    void ClientNotifyTurnStarted(FTurnNotification Notification);

    UFUNCTION(Client, Reliable)
    void ClientNotifyTurnEnded(FTurnNotification Notification);

    UFUNCTION(Client, Reliable)
    void ClientNotifyTurnPaused(FTurnNotification Notification);

    UFUNCTION(Client, Reliable)
    void ClientNotifyTurnResumed(FTurnNotification Notification);

    UFUNCTION(Client, Reliable)
    void ClientNotifyTurnTimedOut(FTurnNotification Notification);

    UFUNCTION(Client, Reliable)
    void ClientNotifyTurnSkipped(FTurnNotification Notification);

    UFUNCTION(Client, Reliable)
    void ClientNotifyForfeited(FTurnNotification Notification);

    // --- Blueprint Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnTurnNotificationDelegate OnTurnStarted;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnTurnNotificationDelegate OnTurnEnded;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnTurnNotificationDelegate OnTurnPaused;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnTurnNotificationDelegate OnTurnResumed;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnTurnNotificationDelegate OnTurnTimedOut;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnTurnNotificationDelegate OnTurnSkipped;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnTurnNotificationDelegate OnForfeited;

    // Fires on ALL participants each turn start
    // bIsMyTurn -- true only for the active participant
    // Action component binds here for cooldown ticking
    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnAnyParticipantTurnStarted OnAnyParticipantTurnStarted;

    // --- Native Delegates --- C++ binding ---

    FOnTurnNotificationDelegate_Native OnTurnStarted_Native;
    FOnTurnNotificationDelegate_Native OnTurnEnded_Native;
    FOnTurnNotificationDelegate_Native OnTurnPaused_Native;
    FOnTurnNotificationDelegate_Native OnTurnResumed_Native;
    FOnTurnNotificationDelegate_Native OnTurnTimedOut_Native;
    FOnTurnNotificationDelegate_Native OnTurnSkipped_Native;
    FOnTurnNotificationDelegate_Native OnForfeited_Native;
    FOnAnyParticipantTurnStarted_Native OnAnyParticipantTurnStarted_Native;

    // --- Called by manager --- not intended for external use ---

    // Called by UTurnBasedParticipantManagerComponent each turn start
    // for ALL participants -- not just the active one
    void HandleAnyParticipantTurnStarted(bool bIsTurn);

protected:

    virtual void BeginPlay() override;

    // Virtual handlers -- subclasses override for custom behaviour
    // AI subclass overrides HandleTurnStarted to run MinMax
    virtual void HandleTurnStarted(const FTurnNotification& Notification);
    virtual void HandleTurnEnded(const FTurnNotification& Notification);
    virtual void HandleTurnPaused(const FTurnNotification& Notification);
    virtual void HandleTurnResumed(const FTurnNotification& Notification);
    virtual void HandleTurnTimedOut(const FTurnNotification& Notification);
    virtual void HandleTurnSkipped(const FTurnNotification& Notification);
    virtual void HandleForfeited(const FTurnNotification& Notification);

private:

    bool bIsMyTurn = false;

    // Helper -- finds manager on game state
    UTurnBasedParticipantManagerComponent* GetParticipantManager() const;
};