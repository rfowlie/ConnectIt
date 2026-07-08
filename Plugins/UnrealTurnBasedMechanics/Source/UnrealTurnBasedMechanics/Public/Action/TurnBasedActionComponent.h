// Fill out your copyright notice in the Description page of Project Settings.

// TurnBasedActionComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TurnBasedMechanicsStructs.h"
#include "TurnBasedAction.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "TurnBasedActionComponent.generated.h"

class UActionLoadOutDataAsset;


// Fired when an action is activated, completed, or cancelled
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionEvent, UTurnBasedAction*, Action);

// Fired when all required actions are complete
// UI can use this to show turn end confirmation
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnEndReady);

// Passthrough delegate -- project wires this to board manager
// Action component has no knowledge of what processes the request
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBoardChangeRequested, const FTurnActionRequest&, Request);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBoardChangeRequested_Native, const FTurnActionRequest&);


UCLASS(ClassGroup=(TurnBased), meta=(BlueprintSpawnableComponent))
class UNREALTURNBASEDMECHANICS_API UTurnBasedActionComponent
    : public UActorComponent
{
    GENERATED_BODY()

public:

    UTurnBasedActionComponent();

    // --- Setup ---

    // Called by controller after finding board config
    // Clones loadout actions into runtime instances
    // Must be called before turn starts
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void InitialiseFromLoadout(UActionLoadOutDataAsset* InLoadOut);

    // --- Turn Lifecycle ---

    // Called by participant component when this participant's turn starts
    // Resets per-turn state and auto-activates required actions
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void OnTurnBegan(int32 InTurnNumber);

    // Called by participant component for ALL participant turn starts
    // Ticks cooldowns -- bIsMyTurn determines cooldown tick behaviour
    void OnAnyParticipantTurnStarted(bool bIsMyTurn);

    // --- Action Control ---

    // Attempt to activate action by tag
    // Cancels current active action if cancellable
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    bool TryActivateAction(FGameplayTag ActionTag);

    // Attempt to activate a specific action instance
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    bool TryActivateActionByRef(UTurnBasedAction* Action);

    // Cancel the currently active action
    // Reactivates required actions after cancellation
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void CancelActiveAction();

    // Whether all required actions are complete
    // Turn cannot end until this returns true
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool CanEndTurn() const;

    // Request turn end -- validates CanEndTurn before submitting
    // Routes through participant component ServerRPC
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

    // --- Delegates ---

    // Project wires this to the board manager
    // ActionComponent has zero knowledge of what receives the request
    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnBoardChangeRequested OnBoardChangeRequested;

    FOnBoardChangeRequested_Native OnBoardChangeRequested_Native;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionEvent OnActionActivated;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionEvent OnActionCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionEvent OnActionCancelled;

    // Fires when all required actions complete
    // UI binds here to show turn end button or auto-end
    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnTurnEndReady OnTurnEndReady;

    // --- Debug ---

    UPROPERTY(BlueprintReadOnly, Category = "Turn Based|Debug")
    TArray<FTurnBasedActionRecord> ActionHistory;

protected:

    virtual void BeginPlay() override;

private:

    // Runtime action instances -- cloned from loadout
    // Owned by this component
    UPROPERTY()
    TArray<UTurnBasedAction*> RuntimeActions;

    // Currently active action -- null when no action is running
    UPROPERTY()
    TObjectPtr<UTurnBasedAction> ActiveAction = nullptr;

    // Source loadout -- not modified at runtime
    UPROPERTY()
    TObjectPtr<UActionLoadOutDataAsset> Loadout = nullptr;

    bool bIsInitialised = false;
    int32 CurrentTurnNumber = 0;

    // --- Initialisation Helpers ---

    void CloneActionsFromLoadOut();
    void BindActionDelegates(UTurnBasedAction* Action);

    // --- Turn Flow ---

    // Auto-activates all required actions that are available
    // Called on turn start and after optional action completes
    void ActivateRequiredActions();

    // Checks if all required actions complete
    // If so fires OnTurnEndReady and calls RequestTurnEnd
    void CheckAutoTurnEnd();

    // --- Action Delegate Handlers ---

    UFUNCTION()
    void HandleActionChangeRequested(const FTurnActionRequest& Request);

    UFUNCTION()
    void HandleActionCompleted(UTurnBasedAction* Action);

    UFUNCTION()
    void HandleActionCancelled(UTurnBasedAction* Action);

    // --- ServerRPC ---

    // Routes FTurnActionRequest to server for validation
    // Valid because UActorComponent can own RPCs
    // Project wiring on server side routes to board manager
    UFUNCTION(Server, Reliable)
    void ServerRouteBoardChangeRequest(FTurnActionRequest Request);
    void ServerRouteBoardChangeRequest_Implementation(
        FTurnActionRequest Request);

    // --- Logging ---

    void LogActionRecord(
        UTurnBasedAction* Action,
        ETurnBasedActionState OutcomeState,
        const FString& Note = TEXT(""));

    // --- Helpers ---

    AController* GetOwningController() const;
    UTurnBasedParticipantComponent* GetParticipantComponent() const;
};