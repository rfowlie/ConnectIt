// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Action/TurnBasedActionBase.h"
#include "Action/TurnBasedAction.h"
#include "Action/TurnBasedSpectatorAction.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "TurnBasedMechanicsStructs.h"
#include "TurnBasedActionsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBoardChangeRequested, const FTurnActionRequest&, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnEndReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnEndRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnActionsEvent, UTurnBasedAction*, Action);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBoardChangeRequested_Native,
    const FTurnActionRequest&);
DECLARE_MULTICAST_DELEGATE(FOnTurnEndRequested_Native);

UCLASS(ClassGroup=(TurnBased), meta=(BlueprintSpawnableComponent))
class UNREALTURNBASEDMECHANICS_API UTurnBasedActionsComponent
    : public UActorComponent
{
    GENERATED_BODY()

public:

    UTurnBasedActionsComponent();

    // --- Behaviour Config ---

    // Sits at bottom of stack during your turn
    // Cannot be popped -- only replaced via ClearAndPush
    // Mandatory -- designers create a do-nothing action if needed
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Behaviour")
    TSubclassOf<UTurnBasedAction> RootActionClass = nullptr;

    // Pushed onto stack immediately after your turn ends
    // Active during Updating phase while resolution plays
    // Replaced by SpectatorViewerActionClass when opponent turn starts
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Behaviour")
    TSubclassOf<UTurnBasedSpectatorAction> IdleViewerActionClass = nullptr;

    // Pushed onto stack when opponent turn starts
    // Active for duration of opponent turn
    // Replaced by RootAction when your turn starts again
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Behaviour")
    TSubclassOf<UTurnBasedSpectatorAction> SpectatorViewerActionClass = nullptr;

    // Pushed on top of stack during system pause
    // Stack preserved below -- restored exactly on unpause
    // Framework provides UTurnBasedPauseAction as default
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Behaviour")
    TSubclassOf<UTurnBasedSpectatorAction> PauseViewerActionClass = nullptr;

    // When true RequestTurnEnd fires automatically when
    // CanAutoEndTurn() returns true after an action completes
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn Based|Behaviour")
    bool bAutoEndTurnOnAllRequiredActionsCompleted = false;

    // --- Setup ---

    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void InitialiseFromLoadout(UActionLoadoutDataAsset* InLoadout);

    // --- Turn Lifecycle ---
    // Called by controller in response to participant component delegates

    // Called when this participant's turn starts
    // Fires OnTurnStarted BlueprintNativeEvent
    // Default implementation clears stack and pushes RootAction
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyTurnStarted(int32 InTurnNumber);

    // Called when opponent's turn starts
    // Fires OnOpponentTurnStarted BlueprintNativeEvent
    // Default implementation clears stack and pushes SpectatorViewerAction
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyOpponentTurnStarted();

    // Called when this participant's turn ends
    // Clears stack and pushes IdleViewerAction
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyTurnEnded();

    // Called on system pause -- pushes PauseViewerAction on top of stack
    // Stack below is preserved and restored on unpause
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyPaused();

    // Called when system pause ends -- pops PauseViewerAction
    // Whatever was below reactivates exactly
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyUnpaused();

    // Called when match ends
    // Clears stack and pushes IdleViewerAction as safe end state
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyMatchEnded();

    // Tick cooldowns on all runtime actions
    // Called internally by NotifyTurnStarted and NotifyOpponentTurnStarted
    // bIsMyTurn drives ShouldTickCooldown per action
    void TickCooldowns(bool bIsMyTurn);

    // --- Stack Control ---

    // Push any action onto the stack
    // Force deactivates current top without popping
    // Activates new top
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void PushAction(UTurnBasedActionBase* Action);

    // Pop top action from stack
    // Reactivates new top
    // Guards against popping last item -- stack never empties during match
    // Returns popped action or null if pop was blocked
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    UTurnBasedActionBase* SafePopAction();

    // Clear entire stack and push a new root
    // Used when ownership changes (turn start, turn end, match end)
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void ClearAndPush(UTurnBasedActionBase* NewRoot);

    // --- Action Control ---

    // Push a runtime action by tag onto the stack
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    bool TryPushAction(FGameplayTag ActionTag);

    // Push a specific runtime action instance onto the stack
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    bool TryPushActionByRef(UTurnBasedAction* Action);

    // Cancel and pop the top action if it is cancellable
    // Returns to whatever is below
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void CancelTopAction();

    // Whether turn can end
    // Calls CanAutoEndTurn() -- designers override that function
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool CanEndTurn() const;

    // Broadcasts OnTurnEndRequested if CanEndTurn() is true
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void RequestTurnEnd();

    // --- Queries ---

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    UTurnBasedActionBase* GetTopAction() const;

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    UTurnBasedActionBase* GetRootAction() const { return RootAction; }

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool IsRootOnTop() const;

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    int32 GetStackDepth() const { return ActionStack.Num(); }

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    TArray<UTurnBasedAction*> GetAllRuntimeActions() const;

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    TArray<UTurnBasedAction*> GetRequiredActions() const;

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    UTurnBasedAction* FindActionByTag(FGameplayTag Tag) const;

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool IsInitialised() const { return bIsInitialised; }

    // --- Delegates ---

    // Board change request passthrough
    // Caller routes to server or board manager
    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnBoardChangeRequested OnBoardChangeRequested;

    FOnBoardChangeRequested_Native OnBoardChangeRequested_Native;

    // Fires when CanAutoEndTurn() becomes true
    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnTurnEndReady OnTurnEndReady;

    // Fires when RequestTurnEnd is called and CanEndTurn() is true
    // Bind to ServerSubmitTurnEnd or equivalent
    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnTurnEndRequested OnTurnEndRequested;

    FOnTurnEndRequested_Native OnTurnEndRequested_Native;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionBaseEvent OnActionPushed;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionBaseEvent OnActionPopped;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnTurnActionsEvent OnActionCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnTurnActionsEvent OnActionCancelled;

    // --- Debug ---

    UPROPERTY(BlueprintReadOnly, Category = "Turn Based|Debug")
    TArray<FTurnBasedActionRecord> ActionHistory;

protected:

    virtual void BeginPlay() override;

    // --- Designer Hooks ---

    // Override to customise turn start behaviour
    // Default: ClearAndPush(RootAction)
    // Subclass: call Super then push additional actions on top
    UFUNCTION(BlueprintNativeEvent, Category = "Turn Based|Actions")
    void OnTurnStarted(int32 TurnNumber);

    // Override to customise opponent turn start behaviour
    // Default: ClearAndPush(SpectatorViewerAction)
    // Subclass: call Super then customise
    UFUNCTION(BlueprintNativeEvent, Category = "Turn Based|Actions")
    void OnOpponentTurnStarted();

    // Override to customise turn end condition
    // Default: all bIsRequired actions have CompletionsThisTurn > 0
    UFUNCTION(BlueprintNativeEvent, Category = "Turn Based|Actions")
    bool CanAutoEndTurn() const;

private:

    // --- Stack ---

    UPROPERTY()
    TArray<TObjectPtr<UTurnBasedActionBase>> ActionStack;

    // --- Runtime Instances ---

    UPROPERTY()
    TArray<UTurnBasedAction*> RuntimeActions;

    // Created from class references in InitialiseFromLoadout
    UPROPERTY()
    TObjectPtr<UTurnBasedAction> RootAction = nullptr;

    UPROPERTY()
    TObjectPtr<UTurnBasedSpectatorAction> IdleViewerAction = nullptr;

    UPROPERTY()
    TObjectPtr<UTurnBasedSpectatorAction> SpectatorViewerAction = nullptr;

    UPROPERTY()
    TObjectPtr<UTurnBasedSpectatorAction> PauseViewerAction = nullptr;

    UPROPERTY()
    TObjectPtr<UActionLoadoutDataAsset> Loadout = nullptr;

    bool bIsInitialised     = false;
    int32 CurrentTurnNumber = 0;

    // Force deactivates and empties the action stack without pushing
    // a replacement -- shared by ClearAndPush and NotifyMatchEnded
    void ClearStack();

    // --- Initialisation ---

    void CloneActionsFromLoadout();
    void CreateSystemActions();
    void BindActionDelegates(UTurnBasedAction* Action);
    void HandleNextActionRequested(TSubclassOf<UTurnBasedAction> NextClass);

    // Logs a warning if a viewer action instance is null -- used by
    // CreateSystemActions to report unset Idle/Spectator viewer classes
    void WarnIfViewerActionMissing(
        const UTurnBasedSpectatorAction* Action,
        const TCHAR* ClassPropertyName,
        const TCHAR* Consequence) const;

    // --- Completion / Auto End ---

    UFUNCTION()
    void HandleActionCompleted(UTurnBasedAction* Action);
    UFUNCTION()
    void HandleActionCancelled(UTurnBasedAction* Action);
    UFUNCTION()
    void HandleBoardChangeRequested(const FTurnActionRequest& Request);
    
    void CheckAutoEndTurn();

    // --- Logging ---

    void LogActionRecord(
        UTurnBasedAction* Action,
        FString Note = TEXT(""));

    AController* GetOwningController() const;
};