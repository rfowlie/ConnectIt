// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TurnBasedMechanicsStructs.h"
#include "TurnBasedParticipantManagerComponent.generated.h"

class UTurnBasedParticipantComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnPhaseChanged, ETurnPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnParticipantForfeited, const FTurnParticipantInfo&, ParticipantInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllParticipantsReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameOver);


UCLASS(ClassGroup=(TurnBased), meta=(BlueprintSpawnableComponent))
class UNREALTURNBASEDMECHANICS_API UTurnBasedParticipantManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	 UTurnBasedParticipantManagerComponent();

    // --- Configuration — set by GameMode before game starts ---

    // How long each turn lasts in seconds
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Turn Based|Config")
    float TurnDuration = 60.f;

    // How many turns missed before forfeit
    UPROPERTY(BlueprintReadOnly, Category = "Turn Based|Config")
    int32 ForfeitThreshold = 3;

    // How long to wait for a disconnected player to reconnect
    UPROPERTY(BlueprintReadOnly, Category = "Turn Based|Config")
    float ReconnectTimeout = 30.f;

    // --- State — replicated for UI ---

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_TurnPhase,
        Category = "Turn Based|State")
    ETurnPhase CurrentPhase = ETurnPhase::WaitingForParticipants;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ActiveParticipantIndex,
        Category = "Turn Based|State")
    int32 ActiveParticipantIndex = -1;

    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Turn Based|State")
    int32 TurnNumber = 0;

    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Turn Based|State")
    TArray<FTurnParticipantInfo> Participants;

    // --- Setup — called by GameMode ---

    // Register a participant — GameMode calls this for each controller
    void RegisterParticipant(
        AController* Controller,
        EParticipantType Type,
        const FString& DisplayName);

    // Runtime swap — callable from Blueprint or GameMode
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Config")
    void SetTurnOrderStrategy(UObject* InStrategy);

    // Called by GameMode when all participants are registered
    // Transitions to WaitingForReady
    void BeginReadyCheck();

    // --- Turn Control --- Server only ---

    // Called when a participant submits turn end
    void NotifyTurnEndSubmitted(AController* Controller);

    // Called by GameMode on player logout
    void NotifyParticipantDisconnected(AController* Controller);

    // Called by GameMode on player login (reconnect)
    void NotifyParticipantReconnected(AController* Controller);

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnTurnPhaseChanged OnTurnPhaseChanged;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnParticipantForfeited OnParticipantForfeited;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnAllParticipantsReady OnAllParticipantsReady;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnGameOver OnGameOver;

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // --- Participant Helpers ---

    FTurnParticipantInfo* FindParticipant(AController* Controller);
    int32 FindParticipantIndex(AController* Controller) const;
    UTurnBasedParticipantComponent* GetParticipantComponent(AController* Controller) const;
    
protected:

    virtual void BeginPlay() override;

    // Instanced — designer picks and configures strategy in Details panel
    // Defaults to Sequential if not set
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Turn | Config")
    TObjectPtr<UObject> TurnOrderStrategy = nullptr;


private:
    // Server side turn timer
    FTimerHandle TurnTimerHandle;

    // Server side reconnect timer — active during Paused phase
    FTimerHandle ReconnectTimerHandle;

    // Index of participant who disconnected — tracked during pause
    int32 DisconnectedParticipantIndex = -1;

    // --- State Machine ---

    void SetPhase(ETurnPhase NewPhase);
    void StartTurn(int32 ParticipantIndex);
    void EndTurn(ETurnEndReason Reason);
    void AdvanceToNextParticipant();
    void HandleTurnTimeout();
    void HandleReconnectTimeout();
    void CheckReadyStatus();
    void CheckGameOver();

    // --- Notifications ---

    void NotifyActiveParticipant(
        ETurnPhase Phase,
        ETurnEndReason Reason = ETurnEndReason::ParticipantEnded);

    FTurnNotification BuildNotification(
        int32 ParticipantIndex,
        ETurnPhase Phase,
        ETurnEndReason Reason = ETurnEndReason::ParticipantEnded) const;

    // --- RepNotify ---

    UFUNCTION()
    void OnRep_TurnPhase();

    UFUNCTION()
    void OnRep_ActiveParticipantIndex();
};
