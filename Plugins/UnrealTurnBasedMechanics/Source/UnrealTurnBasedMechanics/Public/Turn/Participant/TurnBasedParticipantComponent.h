// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TurnBasedMechanicsStructs.h"
#include "TurnBasedParticipantComponent.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnNotification, const FTurnNotification&, Notification);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnNotificationNative, const FTurnNotification&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnyParticipantTurnStart, bool, bIsMyTurn);


UCLASS(ClassGroup=(TurnBased), meta=(BlueprintSpawnableComponent))
class UNREALTURNBASEDMECHANICS_API UTurnBasedParticipantComponent : public UActorComponent
{
	GENERATED_BODY()

public:

    UTurnBasedParticipantComponent();

    // --- Blueprint Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnTurnNotification OnTurnStarted;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnTurnNotification OnTurnEnded;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnTurnNotification OnTurnPaused;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnTurnNotification OnTurnResumed;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnTurnNotification OnTurnTimedOut;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnTurnNotification OnTurnSkipped;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Notifications")
    FOnTurnNotification OnForfeited;

    // --- C++ Native Delegates ---

    FOnTurnNotificationNative OnTurnStarted_Native;
    FOnTurnNotificationNative OnTurnEnded_Native;
    FOnTurnNotificationNative OnTurnPaused_Native;
    FOnTurnNotificationNative OnTurnResumed_Native;
    FOnTurnNotificationNative OnTurnTimedOut_Native;
    FOnTurnNotificationNative OnTurnSkipped_Native;
    FOnTurnNotificationNative OnForfeited_Native;

    // --- Server RPCs ---

    // Called by human participants to signal they are ready to start
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Turn Based")
    void ServerNotifyReady();

    // Called by human participants to end their turn early
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Turn Based")
    void ServerSubmitTurnEnd();

    // --- Client RPCs — called by UTurnBasedParticipantManagerComponent ---

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

    // Whether this is the currently active participant
    UFUNCTION(BlueprintPure, Category = "Turn Based")
    bool IsMyTurn() const { return bIsMyTurn; }

    // Participant type set by manager on registration
    UPROPERTY(BlueprintReadOnly, Category = "Turn Based")
    EParticipantType ParticipantType = EParticipantType::Human;

    UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = "true"))
    int32 PlayerId = -1;

protected:

    virtual void BeginPlay() override;

    // Subclasses override to respond to turn events
    // AI subclass calls ServerSubmitTurnEnd after running MinMax
    virtual void HandleTurnStarted(const FTurnNotification& Notification);
    virtual void HandleTurnEnded(const FTurnNotification& Notification);
    virtual void HandleTurnPaused(const FTurnNotification& Notification);
    virtual void HandleTurnResumed(const FTurnNotification& Notification);
    virtual void HandleTurnTimedOut(const FTurnNotification& Notification);
    virtual void HandleTurnSkipped(const FTurnNotification& Notification);
    virtual void HandleForfeited(const FTurnNotification& Notification);


private:

    bool bIsMyTurn = false;
};
