// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TurnBasedMechanicsStructs.h"
#include "TurnBasedAction.h"
#include "TurnBasedActionComponent.generated.h"

class UActionLoadOutDataAsset;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionEvent, UTurnBasedAction*, Action);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnEndReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnEndRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBoardChangeRequested, const FTurnActionRequest&, Request);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBoardChangeRequested_Native, const FTurnActionRequest&);
DECLARE_MULTICAST_DELEGATE(FOnTurnEndRequested_Native);

UCLASS(ClassGroup=(TurnBased), meta=(BlueprintSpawnableComponent))
class UNREALTURNBASEDMECHANICS_API UTurnBasedActionComponent
    : public UActorComponent
{
    GENERATED_BODY()

public:

    UTurnBasedActionComponent();

    // --- Behaviour Flags ---
    // Configure these per project to match your turn structure

    // When true required actions activate automatically from front
    // to back of the loadout array on turn start and after
    // optional actions complete
    // When false caller drives all activation via TryActivateAction
    // Designers: order your required actions intentionally in the loadout
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Behaviour")
    bool bAutoActivateRequiredActions = true;

    // When true turn end is requested automatically when all
    // required actions complete
    // When false caller must call RequestTurnEnd explicitly
    // Set false when you want explicit player confirmation before
    // ending the turn (e.g. a confirm button in UI)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Behaviour")
    bool bAutoEndTurnOnAllRequiredActionsCompleted = true;

    // --- Setup ---

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void InitialiseFromLoadout(UActionLoadOutDataAsset* InLoadout);

    // --- Turn Lifecycle ---
    // Caller drives these -- no internal bindings to other components

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyTurnStarted(int32 InTurnNumber);

    // Call on every participant turn start -- not just this one
    // bIsMyTurn true only when this participant is active
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void TickCooldowns(bool bIsMyTurn);

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyTurnEnded();

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyTurnPaused();

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyTurnResumed();

    // --- Action Control ---

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    bool TryActivateAction(FGameplayTag ActionTag);

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    bool TryActivateActionByRef(UTurnBasedAction* Action);

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void CancelActiveAction();

    // Returns true when all required actions are complete
    // or when there are no required actions in the loadout
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool CanEndTurn() const;

    // Broadcasts OnTurnEndRequested if CanEndTurn() is true
    // Caller binds OnTurnEndRequested to ServerSubmitTurnEnd or equivalent
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void RequestTurnEnd();

    // --- Queries ---

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    UTurnBasedAction* GetActiveAction() const { return ActiveAction; }

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    TArray<UTurnBasedAction*> GetAllActions() const;

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    TArray<UTurnBasedAction*> GetAvailableActions() const;

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    TArray<UTurnBasedAction*> GetRequiredActions() const;

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    UTurnBasedAction* FindActionByTag(FGameplayTag Tag) const;

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool IsInitialised() const { return bIsInitialised; }

    // Returns true if there are pending required actions not yet complete
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool HasPendingRequiredActions() const
    {
        return !PendingRequiredActions.IsEmpty();
    }

    // --- Delegates ---

    // Fires when an action requests a board change
    // Caller routes this to server or board manager
    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnBoardChangeRequested OnBoardChangeRequested;

    FOnBoardChangeRequested_Native OnBoardChangeRequested_Native;

    // Fires when all required actions complete
    // UI can use this to enable turn end button
    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnTurnEndReady OnTurnEndReady;

    // Fires when RequestTurnEnd is called and CanEndTurn() is true
    // Bind to ServerSubmitTurnEnd or equivalent
    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnTurnEndRequested OnTurnEndRequested;

    FOnTurnEndRequested_Native OnTurnEndRequested_Native;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionEvent OnActionActivated;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionEvent OnActionCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionEvent OnActionCancelled;

    // --- Debug ---

    UPROPERTY(BlueprintReadOnly, Category = "Turn Based|Debug")
    TArray<FTurnBasedActionRecord> ActionHistory;

protected:

    virtual void BeginPlay() override;

private:

    UPROPERTY()
    TArray<UTurnBasedAction*> RuntimeActions;

    UPROPERTY()
    TObjectPtr<UTurnBasedAction> ActiveAction = nullptr;

    // Required actions not yet complete this turn
    // Populated on turn start, advanced as each completes
    // Order matches loadout array order
    UPROPERTY()
    TArray<TObjectPtr<UTurnBasedAction>> PendingRequiredActions;

    UPROPERTY()
    TObjectPtr<UActionLoadOutDataAsset> Loadout = nullptr;

    bool bIsInitialised     = false;
    int32 CurrentTurnNumber = 0;

    void CloneActionsFromLoadout();
    void BindActionDelegates(UTurnBasedAction* Action);

    // Builds PendingRequiredActions and activates first if flag set
    void BuildRequiredQueue();

    // Advances to next action in pending queue
    // Respects bAutoActivateRequiredActions
    void AdvanceRequiredQueue();

    // Fires OnTurnEndReady and optionally RequestTurnEnd
    // based on bAutoEndTurnOnAllRequiredActionsCompleted
    void HandleAllRequiredComplete();

    void CheckAutoTurnEnd();

    UFUNCTION()
    void HandleActionChangeRequested(const FTurnActionRequest& Request);

    UFUNCTION()
    void HandleActionCompleted(UTurnBasedAction* Action);

    UFUNCTION()
    void HandleActionCancelled(UTurnBasedAction* Action);

    void LogActionRecord(
        UTurnBasedAction* Action,
        ETurnBasedActionState OutcomeState,
        const FString& Note = TEXT(""));

    AController* GetOwningController() const;
};