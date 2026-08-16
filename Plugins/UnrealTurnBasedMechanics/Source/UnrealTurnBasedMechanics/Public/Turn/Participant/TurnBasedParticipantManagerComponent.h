// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedMechanicsStructs.h"
#include "Components/ActorComponent.h"
#include "Turn/Order/TurnOrderInterface.h"
#include "TurnBasedParticipantManagerComponent.generated.h"

class ATurnBasedGameState;
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

    UFUNCTION(BlueprintImplementableEvent, Category = "Turn Based|State")
    bool CheckAllParticipantsRegistered();
    
    void RegisterParticipant(AController* Controller, EParticipantType Type);

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    bool IsParticipantRegistered(AController* Controller) const;

    void BeginReadyCheck();

    // --- Turn Control --- Server only ---

    void NotifyTurnEndSubmitted(AController* Controller);
    void NotifyParticipantDisconnected(AController* Controller);
    void NotifyParticipantReconnected(AController* Controller);

    // Called by participant manager when a participant confirms ready
    void NotifyParticipantReady(AController* Controller);

    // Holds AdvanceToNextParticipant back during the resolution phase --
    // external systems bind to OnTurnResolutionStarted and call this
    // immediately if they have gated visual/logic work to finish first,
    // then call EndResolutionHold() once that work is genuinely done.
    // Replaces the old fixed-duration TurnResolutionDuration timer, which
    // only ever estimated how long that work would take instead of knowing.
    void BeginResolutionHold();

    // Releases a hold placed by BeginResolutionHold(). Once the hold count
    // returns to zero (and resolution is still pending), advances to the
    // next participant immediately.
    void EndResolutionHold();

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
    // Cinematics, scoring visuals, dialogue etc. bind here and call
    // BeginResolutionHold()/EndResolutionHold() around their own work.
    // Manager advances immediately once nothing is holding it.
    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnTurnResolutionStarted OnTurnResolutionStarted;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnGameOver OnGameOver;

    // --- Helpers ---

    // returns null on clients
    AController* GetControllerAtIndex(int32 Index) const;

    // Server-authoritative -- true if Controller is the actively-taking-turn
    // participant right now. Safe to call from server-side code that needs
    // to validate a request actually comes from whoever's turn it is; do
    // NOT use UTurnBasedParticipantComponent::IsMyTurn() for this -- that
    // flag is client-side-only (see its own doc comment) and always false
    // on the server.
    UFUNCTION(BlueprintPure, Category = "Turn Based")
    bool IsActiveParticipant(AController* Controller) const
    {
        return IsValid(Controller) && GetControllerAtIndex(ActiveParticipantIndex) == Controller;
    }

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
    FTimerHandle ReconnectTimerHandle;

    int32 DisconnectedParticipantIndex = -1;

    // Count of outstanding BeginResolutionHold() calls not yet matched by
    // EndResolutionHold() -- AdvanceToNextParticipant is deferred while > 0
    int32 ResolutionHoldCount = 0;

    // Set by BeginResolutionHold, reset alongside ResolutionHoldCount at the
    // top of EndTurn. Distinguishes "nobody ever held resolution" from
    // "something held it and already released it synchronously before
    // OnTurnResolutionStarted.Broadcast() returned" -- both look identical
    // as ResolutionHoldCount == 0 to EndTurn's post-broadcast fallback check,
    // but only the first case should trigger it. Without this, a hold taken
    // and released entirely within the broadcast (e.g. a gated tag sequence
    // with nothing registered against it, resolving synchronously) causes
    // EndResolutionHold's own AdvanceToNextParticipant call and EndTurn's
    // fallback call to both fire for the same turn end.
    bool bResolutionHoldTakenThisTurnEnd = false;

    // --- State Machine ---

    void SetPhase(ETurnPhase NewPhase);
    void StartTurn(int32 ParticipantIndex);
    void EndTurn(ETurnEndReason Reason);
   
    void AdvanceToNextParticipant();
    void HandleTurnTimeout();
    void HandleReconnectTimeout();
    void CheckReadyStatus();
    bool CheckGameOver() const;

    // Broadcasts to all participant components each turn start
    // Each component ticks its own cooldowns
    // Each component fires OnAnyParticipantTurnStarted(bIsMyTurn)
    void BroadcastTurnStart(int32 ActiveIndex);
    void BroadcastControllerChanged(int32 ActiveIndex);
    
    // --- Helpers ---

    ATurnBasedGameState* GetOwningGameState() const;
    
    void SetMatchPhase(EMatchPhase NewPhase) const;
    
    FTurnParticipantInfo* FindParticipant(AController* Controller);
    const FTurnParticipantInfo* FindParticipant(AController* Controller) const;
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