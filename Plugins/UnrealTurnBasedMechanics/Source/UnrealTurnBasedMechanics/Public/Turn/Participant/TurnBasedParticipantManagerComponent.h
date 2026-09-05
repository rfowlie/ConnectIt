// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedMechanicsStructs.h"
#include "Components/ActorComponent.h"
#include "Turn/Order/TurnOrderInterface.h"
#include "TurnBasedParticipantManagerComponent.generated.h"

class ATurnBasedGameState;
class UTurnBasedParticipantComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnPhaseChanged, ETurnPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnChanged, const int32, Turn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnParticipantForfeited, const FTurnParticipantInfo&, ParticipantInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActiveControllerChanged, AController*, NewActiveController);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnParticipantIndexChanged, const int32, ParticipantIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllParticipantsReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameOver);

// Everything a debug widget needs to know about this component's current
// values in one call -- used to seed initial state once, right after
// binding, through the same events used for later reactive updates (see
// UDWidgetBase's own class comment for the convention this follows).
// Deliberately carries ActiveParticipantIndex (int32), not a controller
// reference -- OnActiveControllerChanged hands out AController*, but a
// widget only ever wants the index, so the translation happens once here
// rather than leaking the controller pointer itself.
USTRUCT(BlueprintType)
struct FTurnBasedParticipantManagerInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    ETurnPhase TurnPhase = ETurnPhase::WaitingForParticipants;

    UPROPERTY(BlueprintReadOnly)
    int32 ActiveParticipantIndex = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 TurnNumber = 0;

    UPROPERTY(BlueprintReadOnly)
    TArray<FTurnParticipantInfo> Participants;
};

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

    // ReplicatedUsing rather than plain Replicated -- TurnNumber is
    // monotonic and never repeats within a match, so unlike CurrentPhase it
    // can never "settle back" to a value the client already has and go
    // unsent for a tick. Gives listeners (e.g. a debug UI) a signal that's
    // guaranteed to fire on every single turn advance, independent of
    // whichever unrelated RPC/property happens to also be in flight that
    // tick. See OnRep_TurnNumber.
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_TurnNumber, Category = "Turn Based|State")
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

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnTurnPhaseChanged OnTurnPhaseChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnTurnChanged OnTurnChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnActiveControllerChanged OnActiveControllerChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnParticipantForfeited OnParticipantForfeited;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnParticipantIndexChanged OnParticipantIndexChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnAllParticipantsReady OnAllParticipantsReady;

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

    // Everything a debug widget needs, in one call -- see
    // FTurnBasedParticipantManagerInfo's own comment.
    UFUNCTION(BlueprintPure, Category = "Turn Based|Debug")
    FTurnBasedParticipantManagerInfo GetInfo() const
    {
        return { CurrentPhase, ActiveParticipantIndex, TurnNumber, Participants };
    }

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

    virtual void BeginPlay() override;

    // Tag triggered on UGameEventTaskSubsystem when a turn ends -- external
    // systems register gated async tasks against this tag (same pattern as
    // any other gated sequence step) instead of a bespoke hold API. Must be
    // set by the project (this plugin has no opinion on the tag's name) --
    // AdvanceToNextParticipant only runs once every registered task for
    // this tag completes, or immediately if none are registered.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turn Based|Config")
    FGameplayTag TurnEndEventTag;

private:

    // Server only -- index matched to Participants
    // Never replicated -- controllers are not network relevant to other clients
    TArray<TWeakObjectPtr<AController>> ServerControllers;
    
    // --- Timers ---
    FTimerHandle TurnTimerHandle;
    FTimerHandle ReconnectTimerHandle;

    int32 DisconnectedParticipantIndex = -1;

    // --- State Machine ---

    void SetPhase(ETurnPhase NewPhase);
    void StartTurn(int32 ParticipantIndex);
    void EndTurn(ETurnEndReason Reason);

    // UFUNCTION() -- bound to TurnEndEventTag's OnManagerComplete via
    // BindOnTagComplete (reflection-based, needs a UFUNCTION to find it).
    // Tag param is unused -- this only ever binds to TurnEndEventTag, so
    // there's nothing to disambiguate -- but the signature must match
    // OnManagerComplete's (FGameplayTag) exactly, since BindOnTagComplete's
    // underlying FScriptDelegate::BindUFunction bind is not compile-time
    // signature-checked; a mismatch here would misbehave at runtime, not
    // fail to build.
    UFUNCTION()
    void AdvanceToNextParticipant(FGameplayTag Tag);

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

    UFUNCTION()
    void OnRep_TurnNumber();
};