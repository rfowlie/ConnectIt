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

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBoardChangeRequested_Native, const FTurnActionRequest&);
DECLARE_MULTICAST_DELEGATE(FOnTurnEndRequested_Native);

// Safe, copied stand-in for the live UTurnBasedActionBase*/UTurnBasedAction*
// OnActionPushed/OnActionPopped/OnActionCompleted/OnActionCancelled hand
// out -- observer-style consumers (debug widgets) that only ever need to
// know WHICH action, not a mutable reference TO it (Complete()/Cancel()
// live on the real object), bind the OnAction*Safe siblings below instead.
USTRUCT(BlueprintType)
struct FTurnActionSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Turn Based|Debug")
    FGameplayTag ActionTag;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionStackSnapshotChanged, const FTurnActionSnapshot&, Snapshot);

// Everything a debug widget needs to know about this component's current
// state in one call -- used to seed initial values once, right after
// binding, through the same events used for later reactive updates (see
// UDWidgetBase's own class comment for the convention this follows).
USTRUCT(BlueprintType)
struct FTurnBasedActionsComponentInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Turn Based|Debug")
    FGameplayTag TopActionTag;

    UPROPERTY(BlueprintReadOnly, Category = "Turn Based|Debug")
    FGameplayTag RootActionTag;

    UPROPERTY(BlueprintReadOnly, Category = "Turn Based|Debug")
    int32 StackDepth = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Turn Based|Debug")
    bool bAwaitingRequestConfirmation = false;
};

UCLASS(ClassGroup=(TurnBased), meta=(BlueprintSpawnableComponent))
class UNREALTURNBASEDMECHANICS_API UTurnBasedActionsComponent : public UActorComponent
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

    // Pushed while a board-change request this participant just sent is in
    // flight, blocking further stack mutation until the server answers via
    // NotifyBoardChangeOutcome. Conceptually distinct from
    // IdleViewerActionClass (turn already ended) -- this is mid-turn,
    // waiting on the participant's own pending request.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Behaviour")
    TSubclassOf<UTurnBasedSpectatorAction> AwaitingConfirmationActionClass = nullptr;

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
    void NotifyTurnStarted(const FTurnStartContext& Context);

    // Called when opponent's turn starts
    // Fires OnOpponentTurnStarted BlueprintNativeEvent
    // Default implementation clears stack and pushes SpectatorViewerAction
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyOpponentTurnStarted(const FTurnStartContext& Context);

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

    // --- Board Change Confirmation ---

    // The only thing project-specific glue needs to call once the server's
    // answer to a board-change request is known -- e.g. from a Client RPC
    // fired by the project's own PlayerController. No-ops if Request doesn't
    // match whatever this component is currently awaiting (see
    // HandleBoardChangeRequested). On success, force-completes the action
    // that made the request; on failure, simply reactivates it so the
    // player can retry -- neither this component nor the plugin needs to
    // know *why* a request succeeded or failed, only that it did.
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void NotifyBoardChangeOutcome(const FTurnActionRequest& Request, bool bSucceeded);

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool IsAwaitingRequestConfirmation() const { return bAwaitingRequestConfirmation; }

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

    // Safe siblings of the four above -- broadcast alongside them (same
    // call site, same moment), carrying a copied FTurnActionSnapshot
    // instead of the live action pointer. Observer-style consumers (debug
    // widgets) should bind these, not the raw-pointer originals above.
    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Debug")
    FOnActionStackSnapshotChanged OnActionPushedSafe;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Debug")
    FOnActionStackSnapshotChanged OnActionPoppedSafe;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Debug")
    FOnActionStackSnapshotChanged OnActionCompletedSafe;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Debug")
    FOnActionStackSnapshotChanged OnActionCancelledSafe;

    // --- Debug ---

    UPROPERTY(BlueprintReadOnly, Category = "Turn Based|Debug")
    TArray<FTurnBasedActionRecord> ActionHistory;

    // Everything a debug widget needs, in one call -- see
    // FTurnBasedActionsComponentInfo's own comment.
    UFUNCTION(BlueprintPure, Category = "Turn Based|Debug")
    FTurnBasedActionsComponentInfo GetInfo() const;

protected:

    virtual void BeginPlay() override;

    // --- Designer Hooks ---

    // Override to customise turn start behaviour
    // Default: ClearAndPush(RootAction)
    // Subclass: call Super then push additional actions on top
    UFUNCTION(BlueprintNativeEvent, Category = "Turn Based|Actions")
    void OnTurnStarted(const FTurnStartContext& Context);

    // Override to customise opponent turn start behaviour
    // Default: ClearAndPush(SpectatorViewerAction)
    // Subclass: call Super then customise
    UFUNCTION(BlueprintNativeEvent, Category = "Turn Based|Actions")
    void OnOpponentTurnStarted(const FTurnStartContext& Context);

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
    TObjectPtr<UTurnBasedSpectatorAction> AwaitingConfirmationAction = nullptr;

    UPROPERTY()
    TObjectPtr<UActionLoadoutDataAsset> Loadout = nullptr;

    bool bIsInitialised     = false;
    int32 CurrentTurnNumber = 0;

    // --- Board Change Confirmation ---
    // Set by HandleBoardChangeRequested when AwaitingConfirmationAction is
    // pushed; cleared by NotifyBoardChangeOutcome before it mutates the
    // stack itself. While true, PushAction/SafePopAction/TryPushActionByRef/
    // ClearAndPush all refuse to run -- see each for the guard.
    bool bAwaitingRequestConfirmation = false;
    FTurnActionRequest PendingRequest;

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