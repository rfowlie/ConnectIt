// Fill out your copyright notice in the Description page of Project Settings.

// TurnBasedActionsComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TurnBasedAction.h"
#include "ActionLoadoutDataAsset.h"
#include "TurnBasedMechanicsStructs.h"
#include "TurnBasedActionsComponent.generated.h"

class UActionLoadOutDataAsset;
class UTurnBasedViewerAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionEvent, UTurnBasedAction*, Action);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnViewerActionEvent, UTurnBasedViewerAction*, Action);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnEndReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnEndRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBoardChangeRequested, const FTurnActionRequest&, Request);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBoardChangeRequested_Native, const FTurnActionRequest&);
DECLARE_MULTICAST_DELEGATE(FOnTurnEndRequested_Native);

UCLASS(ClassGroup=(TurnBased), meta=(BlueprintSpawnableComponent))
class UNREALTURNBASEDMECHANICS_API UTurnBasedActionsComponent
    : public UActorComponent
{
    GENERATED_BODY()

public:

    UTurnBasedActionsComponent();

    // --- Behaviour Configuration ---

    // Active during YOUR turn when no board action is running
    // Reactivated after every board action completes or cancels
    // Interrupted silently when a board action activates
    // Leave null if your game auto-activates required actions instead
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Behaviour")
    TSubclassOf<UTurnBasedAction> RootActionClass = nullptr;

    // Active when it is NOT your turn
    // Deactivated when your turn starts -- root action takes over
    // Never completes naturally -- deactivated by external signal
    // Provides passive input handling (camera, cursor, menus, observer UI)
    // Leave null if no passive behaviour is needed when not your turn
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Behaviour")
    TSubclassOf<UTurnBasedViewerAction> ViewerActionClass = nullptr;

    // When true required actions auto-activate from front to back
    // of the loadout array on turn start
    // Designers: order required actions intentionally in the loadout
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Behaviour")
    bool bAutoActivateRequiredActions = false;

    // When true turn end is requested automatically when all
    // required actions complete
    // Set false when explicit player confirmation is needed
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Behaviour")
    bool bAutoEndTurnOnAllRequiredActionsCompleted = false;

    // --- Setup ---

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void InitialiseFromLoadout(UActionLoadOutDataAsset* InLoadout);

    // --- Turn Lifecycle ---
    // Caller drives these -- no internal bindings to other components

    // Called when this participant's turn starts
    // Deactivates viewer action, activates root, builds required queue
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyTurnStarted(int32 InTurnNumber);

    // Called on every participant turn start -- not just this one
    // Drives cooldown ticking for all actions
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void TickCooldowns(bool bIsMyTurn);

    // Called when this participant's turn ends
    // Deactivates root and board actions, activates viewer action
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyTurnEnded();

    // Called when turn is paused (e.g. disconnect)
    // Cancels active board action but preserves viewer state
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyTurnPaused();

    // Called when turn resumes after pause
    // Rebuilds required queue and reactivates root
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyTurnResumed();

    // Called when match ends -- deactivates both root and viewer
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyMatchEnded();

    // --- Action Control ---

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    bool TryActivateAction(FGameplayTag ActionTag);

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    bool TryActivateActionByRef(UTurnBasedAction* Action);

    // Cancels active board action and returns to root
    // Has no effect if root or viewer is currently active
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void CancelActiveAction();

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool CanEndTurn() const;

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void RequestTurnEnd();

    // --- Queries ---

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    UTurnBasedAction* GetActiveAction() const { return ActiveAction; }

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    UTurnBasedViewerAction* GetViewerAction() const { return ViewerAction; }

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool IsRootActionActive() const
    {
        return IsValid(RootAction) && ActiveAction == RootAction;
    }

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool IsViewerActionActive() const { return bViewerActionActive; }

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    UTurnBasedAction* GetRootAction() const { return RootAction; }

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

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool HasPendingRequiredActions() const
    {
        return !PendingRequiredActions.IsEmpty();
    }

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnBoardChangeRequested OnBoardChangeRequested;

    FOnBoardChangeRequested_Native OnBoardChangeRequested_Native;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnTurnEndReady OnTurnEndReady;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnTurnEndRequested OnTurnEndRequested;

    FOnTurnEndRequested_Native OnTurnEndRequested_Native;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionEvent OnActionActivated;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionEvent OnActionCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionEvent OnActionCancelled;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnViewerActionEvent OnViewerActionActivated;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnViewerActionEvent OnViewerActionDeactivated;

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

    // Root action instance -- managed separately from RuntimeActions
    UPROPERTY()
    TObjectPtr<UTurnBasedAction> RootAction = nullptr;

    // Viewer action instance -- single slot, never queued
    UPROPERTY()
    TObjectPtr<UTurnBasedViewerAction> ViewerAction = nullptr;

    UPROPERTY()
    TArray<TObjectPtr<UTurnBasedAction>> PendingRequiredActions;

    UPROPERTY()
    TObjectPtr<UActionLoadOutDataAsset> Loadout = nullptr;

    bool bIsInitialised      = false;
    bool bViewerActionActive = false;
    int32 CurrentTurnNumber  = 0;

    // --- Initialisation ---

    void CloneActionsFromLoadout();
    void CreateRootAction();
    void CreateViewerAction();
    void BindActionDelegates(UTurnBasedAction* Action);

    // --- Viewer Action ---

    void ActivateViewerAction();
    void DeactivateViewerAction();

    // --- Root Action ---

    void ReturnToRoot();

    // --- Required Queue ---

    void BuildRequiredQueue();
    void AdvanceRequiredQueue();
    void HandleAllRequiredComplete();
    void CheckRequiredCompletion();

    // --- Action Delegate Handlers ---

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