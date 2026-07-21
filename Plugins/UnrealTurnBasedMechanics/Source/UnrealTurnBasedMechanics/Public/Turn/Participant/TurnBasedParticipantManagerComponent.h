// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedMechanicsStructs.h"
#include "Components/ActorComponent.h"
#include "Turn/Order/TurnOrderInterface.h"
#include "TurnBasedParticipantManagerComponent.generated.h"

class UTurnBasedParticipantComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnPhaseChanged, ETurnPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnParticipantForfeited, const FTurnParticipantInfo&, ParticipantInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActiveControllerChanged, AController*, NewActiveController);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllParticipantsReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnResolutionStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameOver);

UCLASS(ClassGroup=(TurnBased), meta=(BlueprintSpawnableComponent))
class UNREALTURNBASEDMECHANICS_API UTurnBasedParticipantManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UTurnBasedParticipantManagerComponent();

    // --- Configuration ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Turn Based|Config")
    float TurnDuration = 60.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Turn Based|Config")
    int32 ForfeitThreshold = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Turn Based|Config")
    float ReconnectTimeout = 30.f;

    // Duration of the resolution phase between TurnEnd and next TurnStart
    // External systems hook OnTurnResolutionStarted to drive their logic
    // Manager advances to next turn after this duration automatically
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Turn Based|Config")
    float TurnResolutionDuration = 2.0f;

    // Turn order strategy -- must implement ITurnOrderInterface
    // Instanced inline in Details panel
    // Defaults to USequentialTurnOrderStrategy if not set
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turn Based|Config",
        meta = (MustImplement = "/Script/UnrealTurnBasedMechanics.TurnOrderInterface"))
    TScriptInterface<ITurnOrderInterface> TurnOrderStrategy;

    // --- Replicated State ---

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentPhase, Category = "Turn Based|State")
    ETurnPhase CurrentPhase = ETurnPhase::WaitingForParticipants;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ActiveParticipantIndex, Category = "Turn Based|State")
    int32 ActiveParticipantIndex = -1;

    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Turn Based|State")
    int32 TurnNumber = 0;

    // Replicated -- what clients need to see
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Turn Based|State")
    TArray<FTurnParticipantInfo> Participants;
    
    
    // Replicated once per turn start -- clients start local timer from this
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Turn Based|State")
    float ReplicatedTurnDuration = 0.f;

    // --- Setup --- Server only ---

    void RegisterParticipant(
        AController* Controller,
        EParticipantType Type,
        const FString& DisplayName);

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    bool IsParticipantRegistered(AController* Controller) const;

    void BeginReadyCheck();

    // --- Turn Control --- Server only ---

    void NotifyTurnEndSubmitted(AController* Controller);
    void NotifyParticipantDisconnected(AController* Controller);
    void NotifyParticipantReconnected(AController* Controller);

    // Called by participant manager when a participant confirms ready
    void NotifyParticipantReady(AController* Controller);

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnTurnPhaseChanged OnTurnPhaseChanged;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnActiveControllerChanged OnActiveControllerChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnParticipantForfeited OnParticipantForfeited;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnAllParticipantsReady OnAllParticipantsReady;

    // Hook for external systems during resolution phase
    // Cinematics, scoring visuals, dialogue etc. bind here
    // Manager auto-advances after TurnResolutionDuration
    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnTurnResolutionStarted OnTurnResolutionStarted;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnGameOver OnGameOver;

    // --- Helpers ---
    
    // returns null on clients
    AController* GetControllerAtIndex(int32 Index) const;
    
    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

    virtual void BeginPlay() override;

private:

    // Server only -- index matched to Participants
    // Never replicated -- controllers are not network relevant to other clients
    TArray<TWeakObjectPtr<AController>> ServerControllers;
    
    // --- Timers ---
    FTimerHandle TurnTimerHandle;
    FTimerHandle ResolutionTimerHandle;
    FTimerHandle ReconnectTimerHandle;

    int32 DisconnectedParticipantIndex = -1;

    // --- State Machine ---

    void SetPhase(ETurnPhase NewPhase);
    void StartTurn(int32 ParticipantIndex);
    void EndTurn(ETurnEndReason Reason);
   
    void AdvanceToNextParticipant();
    void HandleTurnTimeout();
    void HandleResolutionComplete();
    void HandleReconnectTimeout();
    void CheckReadyStatus();
    void CheckGameOver();

    // Broadcasts to all participant components each turn start
    // Each component ticks its own cooldowns
    // Each component fires OnAnyParticipantTurnStarted(bIsMyTurn)
    void BroadcastTurnStart(int32 ActiveIndex);
    void BroadcastControllerChanged(int32 ActiveIndex);
    
    // --- Helpers ---
    
    FTurnParticipantInfo* FindParticipant(AController* Controller);
    int32 FindParticipantIndex(AController* Controller) const;
    UTurnBasedParticipantComponent* GetParticipantComponent(
        AController* Controller) const;
    
    void NotifyActiveParticipant(
        ETurnPhase Phase,
        ETurnEndReason Reason = ETurnEndReason::ParticipantEnded);

    FTurnNotification BuildNotification(
        int32 ParticipantIndex,
        ETurnPhase Phase,
        ETurnEndReason Reason = ETurnEndReason::ParticipantEnded) const;

    // --- RepNotify ---

    UFUNCTION()
    void OnRep_CurrentPhase();

    UFUNCTION()
    void OnRep_ActiveParticipantIndex();
};