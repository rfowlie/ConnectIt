// Fill out your copyright notice in the Description page of Project Settings.

// TurnBasedActionComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TurnBasedAction.h"
#include "TurnBasedMechanicsStructs.h"
#include "TurnBasedActionComponent.generated.h"

class UActionLoadOutDataAsset;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActionRequired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionActivated, UTurnBasedAction*, Action);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionCompleted, UTurnBasedAction*, Action);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionCancelled, UTurnBasedAction*, Action);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnEndReady);


UCLASS(ClassGroup=(TurnBased), meta=(BlueprintSpawnableComponent))
class UNREALTURNBASEDMECHANICS_API UTurnBasedActionComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UTurnBasedActionComponent();

    // --- Setup ---

    // Called by the controller after finding the board config actor
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void InitialiseFromLoadOut(UActionLoadOutDataAsset* InDataAsset);

    // --- Turn Lifecycle ---

    // Called by participant component when this participant's turn starts
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void OnTurnBegan();

    // Called by participant manager each turn start for ALL participants
    // bIsMyTurn — whether this is the owning controller's turn
    void OnAnyParticipantTurnStarted(bool bIsMyTurn);

    // --- Action Control ---

    // Attempt to activate an action by tag
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    bool TryActivateAction(FGameplayTag ActionTag);

    // Attempt to activate a specific action instance
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    bool TryActivateActionByRef(UTurnBasedAction* Action);

    // Cancel the currently active action
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void CancelActiveAction();

    // Whether all required actions are complete
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool CanEndTurn() const;

    // Request turn end — validates CanEndTurn before submitting
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Actions")
    void RequestTurnEnd();

    // --- Queries ---

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    UTurnBasedAction* GetActiveAction() const { return ActiveAction; }

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    TArray<UTurnBasedAction*> GetAvailableActions() const;

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    UTurnBasedAction* FindActionByTag(FGameplayTag Tag) const;

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionRequired OnActionRequired;
    
    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionActivated OnActionActivated;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionCompleted OnActionCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionCancelled OnActionCancelled;

    // Fires when all required actions are complete and turn can end
    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnTurnEndReady OnTurnEndReady;

    // Debug log — all action records this session
    UPROPERTY(BlueprintReadOnly, Category = "Turn Based|Debug")
    TArray<FTurnBasedActionRecord> ActionHistory;

protected:

    virtual void BeginPlay() override;

private:

    // Cloned runtime instances — owned by this component
    UPROPERTY()
    TArray<TObjectPtr<UTurnBasedAction>> ActiveActions;

    UPROPERTY()
    TObjectPtr<UTurnBasedAction> ActiveAction = nullptr;

    // Source loadout — not modified at runtime
    UPROPERTY()
    TObjectPtr<UActionLoadOutDataAsset> Loadout = nullptr;

    // Cached turn number for logging
    int32 CurrentTurnNumber = 0;

    // Clone loadout actions into runtime instances
    void CloneActionsFromLoadOut();

    // Wire action delegates
    void BindActionDelegates(UTurnBasedAction* Action);

    // Handlers
    UFUNCTION()
    void HandleActionCompleted(UTurnBasedAction* Action);

    UFUNCTION()
    void HandleActionCancelled(UTurnBasedAction* Action);

    // Log a record for debug
    void LogActionRecord(
        UTurnBasedAction* Action,
        ETurnBasedActionState OutcomeState,
        const FString& Note = TEXT(""));

    // Check if all required actions are done and auto-end if so
    void CheckAutoTurnEnd();
};