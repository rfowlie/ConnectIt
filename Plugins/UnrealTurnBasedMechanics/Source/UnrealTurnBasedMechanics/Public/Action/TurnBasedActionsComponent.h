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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionsEvent, UTurnBasedAction*, Action);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActionTurnEndReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActionTurnEndRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionBoardChangeRequested, const FTurnActionRequest&, Request);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBoardChangeRequested_Native, const FTurnActionRequest&);
DECLARE_MULTICAST_DELEGATE(FOnTurnEndRequested_Native);

UCLASS(ClassGroup=(TurnBased), meta=(BlueprintSpawnableComponent))
class UNREALTURNBASEDMECHANICS_API UTurnBasedActionsComponent
    : public UActorComponent
{
    GENERATED_BODY()

public:

    UTurnBasedActionsComponent();

    // --- Behaviour Flags ---

    // Action that is always active when no other action is running
    // Activated on turn start and after any action completes or cancels
    // Cannot be cancelled by the player -- only interrupted by other actions
    // Leave null if your game has no meaningful idle/default state
    // Example Gigafire: UNavigateSceneAction (camera, cursor, hover)
    // Example ConnectIt: null (PlacePiece auto-activates instead)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn Based|Behaviour")
    TSubclassOf<UTurnBasedAction> RootActionClass = nullptr;

    // When true required actions auto-activate over the root action
    // in loadout array order
    // When false caller drives all activation explicitly
    // Designers: order your required actions intentionally in the loadout
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Behaviour")
    bool bAutoActivateRequiredActions = false;

    // When true turn end is requested automatically when all required
    // actions complete
    // When false caller must call RequestTurnEnd explicitly
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Behaviour")
    bool bAutoEndTurnOnAllRequiredActionsCompleted = false;

    // --- Setup ---

    // set the available actions, can be helpful for forcing actions, tutorials, etc.
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void InitialiseFromLoadout(UActionLoadOutDataAsset* InLoadout);

    // --- Turn Lifecycle ---
    // Caller drives these -- no internal bindings to other components

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyTurnStarted(int32 InTurnNumber);

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyTurnEnded();

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyTurnPaused();

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyTurnResumed();

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void TickCooldowns(bool bIsMyTurn);
    
    // --- Action Control ---

    // Attempts to activate an action by tag
    // If a root action is active it will be interrupted
    // If a non-root non-cancellable action is active this fails
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    bool TryActivateActionByTag(FGameplayTag ActionTag);

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    bool TryActivateActionByRef(UTurnBasedAction* Action);

    // Cancels the active action and returns to root
    // Has no effect if root action is currently active
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void CancelActiveAction();

    // Returns true when all required actions complete
    // or when loadout has no required actions
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool CanEndTurn() const;

    // Broadcasts OnTurnEndRequested if CanEndTurn() is true
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void RequestTurnEnd();

    // --- Queries ---

    // Returns active action -- never null after turn starts if root is set
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    UTurnBasedAction* GetActiveAction() const { return ActiveAction; }

    // Returns true if root action is currently active
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool IsRootActionActive() const
    {
        return IsValid(RootAction) && ActiveAction == RootAction;
    }

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
    FOnActionBoardChangeRequested OnBoardChangeRequested;

    FOnBoardChangeRequested_Native OnBoardChangeRequested_Native;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionTurnEndReady OnTurnEndReady;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionTurnEndRequested OnTurnEndRequested;

    FOnTurnEndRequested_Native OnTurnEndRequested_Native;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionsEvent OnActionActivated;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionsEvent OnActionCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionsEvent OnActionCancelled;

    // --- Debug ---

    UPROPERTY(BlueprintReadOnly, Category = "Turn Based|Debug")
    TArray<FTurnBasedActionRecord> ActionHistory;

protected:

    virtual void BeginPlay() override;

private:

    // Runtime instances cloned from loadout
    UPROPERTY()
    TArray<UTurnBasedAction*> RuntimeActions;

    // The active action -- never null after turn starts if RootActionClass set
    UPROPERTY()
    TObjectPtr<UTurnBasedAction> ActiveAction = nullptr;

    // Runtime root action instance -- cloned from RootActionClass
    // Not part of RuntimeActions -- managed separately
    UPROPERTY()
    TObjectPtr<UTurnBasedAction> RootAction = nullptr;

    // Required actions not yet complete this turn
    // Order matches loadout array order
    UPROPERTY()
    TArray<TObjectPtr<UTurnBasedAction>> PendingRequiredActions;

    UPROPERTY()
    TObjectPtr<UActionLoadOutDataAsset> Loadout = nullptr;

    bool bIsInitialised     = false;
    int32 CurrentTurnNumber = 0;

    // --- Internal ---

    void CloneActionsFromLoadout();
    void CreateRootAction();
    void BindActionDelegates(UTurnBasedAction* Action);

    // Activates root action if one exists
    // Called after any action completes or cancels
    // Called on turn start if bAutoActivateRequiredActions is false
    //   or no pending required actions remain
    void ReturnToRoot();

    // Builds pending required queue and optionally auto-activates
    void BuildRequiredQueue();

    // Advances required queue -- interrupts root if needed
    void AdvanceRequiredQueue();

    // Called when all required actions are complete
    void HandleAllRequiredComplete();

    // Checks whether all required actions are done
    // Called after required action completes when not auto-activating
    void CheckRequiredCompletion();

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