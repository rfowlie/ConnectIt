// Fill out your copyright notice in the Description page of Project Settings.

// TurnBasedAction.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedMechanicsStructs.h"
#include "TurnBasedAction.generated.h"

class AGridTileBase;
class UGridWorldSubsystem;
class UEnhancedInputComponent;
class UInputAction;
class UTurnBasedAction;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnBasedActionEvent, UTurnBasedAction*, Action);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnActionRequested, const FTurnActionRequest&, Request);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnBasedActionEvent_Native, UTurnBasedAction*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnActionRequested_Native, const FTurnActionRequest&);

UCLASS(Abstract, Blueprintable, BlueprintType,
    EditInlineNew, DefaultToInstanced)
class UNREALTURNBASEDMECHANICS_API UTurnBasedAction : public UObject
{
    GENERATED_BODY()

public:

    // --- Designer Config ---
    // Set inline in UParticipantActionLoadout DataAsset

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    FGameplayTag ActionTag;

    // Turn cannot end without this action completing
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    bool bIsRequired = false;

    // Can be cancelled once active
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    bool bIsCancellable = true;

    // Whether an optional action can interrupt this action while it is active
    // Only relevant for required actions
    // When false -- optional actions cannot activate while this is the active action
    // When true  -- optional actions can interupt this and activate
    //               this action reactivates when the optional action completes
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    bool bAllowsOptionalInterrupt = true;
    
    // Waits for a grid tile selection before completing
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    bool bRequiresSelection = false;

    // How many times usable per turn -- 0 = unlimited
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config",
        meta = (ClampMin = 0))
    int32 MaxUsesPerTurn = 1;

    // Turns before usable again after use -- 0 = no cooldown
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config",
        meta = (ClampMin = 0))
    int32 CooldownTurns = 0;

    // Enhanced Input action for confirming selection
    // Set per project -- injected via InitialiseAction
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    TObjectPtr<UInputAction> SelectionInputAction = nullptr;

    // --- Runtime State ---

    UPROPERTY(BlueprintReadOnly, Category = "Action|State")
    ETurnBasedActionState State = ETurnBasedActionState::Available;

    UPROPERTY(BlueprintReadOnly, Category = "Action|State")
    int32 UsesThisTurn = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Action|State")
    int32 TurnsUntilAvailable = 0;

    // --- Lifecycle ---

    // Called by UTurnBasedActionComponent after cloning
    // Provides owning controller and input component references
    // Must be called before Activate
    void InitialiseAction(
        AController* InOwningController,
        UEnhancedInputComponent* InInputComponent);

    UFUNCTION(BlueprintCallable, Category = "Action")
    void Activate();

    UFUNCTION(BlueprintCallable, Category = "Action")
    void Cancel();

    // Called by subclass when the action is finished
    // Fires OnActionCompleted and cleans up selection bindings
    UFUNCTION(BlueprintCallable, Category = "Action")
    void Complete();

    UFUNCTION(BlueprintPure, Category = "Action")
    bool CanActivate() const;

    UFUNCTION(BlueprintPure, Category = "Action")
    bool IsComplete() const
    {
        return State == ETurnBasedActionState::Completed;
    }

    // Called by action component each turn start
    // bIsOwningParticipantTurn -- whether this is the owning controller's turn
    void TickCooldown(bool bIsOwningParticipantTurn);

    // Resets per-turn state -- called at start of each owning turn
    void ResetTurnState();

    // --- Delegates ---

    // Fires when the action wants to request a board change
    // UTurnBasedActionComponent receives this and routes to server
    // Action never touches the board manager directly
    UPROPERTY(BlueprintAssignable, Category = "Action|Delegates")
    FOnTurnActionRequested OnChangeRequested;

    FOnTurnActionRequested_Native OnChangeRequested_Native;

    UPROPERTY(BlueprintAssignable, Category = "Action|Delegates")
    FOnTurnBasedActionEvent OnActionActivated;

    UPROPERTY(BlueprintAssignable, Category = "Action|Delegates")
    FOnTurnBasedActionEvent OnActionCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Action|Delegates")
    FOnTurnBasedActionEvent OnActionCancelled;

    FOnTurnBasedActionEvent_Native OnActionActivated_Native;
    FOnTurnBasedActionEvent_Native OnActionCompleted_Native;
    FOnTurnBasedActionEvent_Native OnActionCancelled_Native;

    // --- World Access ---

    virtual UWorld* GetWorld() const override;

protected:

    // --- Virtual Lifecycle Hooks ---
    // Subclasses override these for concrete behaviour

    UFUNCTION(BlueprintNativeEvent, Category = "Action")
    void OnActivated();
    virtual void OnActivated_Implementation();

    UFUNCTION(BlueprintNativeEvent, Category = "Action")
    void OnCancelled();
    virtual void OnCancelled_Implementation();

    UFUNCTION(BlueprintNativeEvent, Category = "Action")
    void OnCompleted();
    virtual void OnCompleted_Implementation();

    // --- Virtual Selection Hooks ---
    // Subclasses define what counts as a valid hover or selection
    // No visual responsibilities here -- visuals are handled
    // by tile actors responding to GameplayTags sent by subclasses

    UFUNCTION(BlueprintNativeEvent, Category = "Action|Selection")
    bool IsValidHoverTile(AGridTileBase* Tile) const;
    virtual bool IsValidHoverTile_Implementation(
        AGridTileBase* Tile) const;

    UFUNCTION(BlueprintNativeEvent, Category = "Action|Selection")
    bool IsValidSelectionTile(AGridTileBase* Tile) const;
    virtual bool IsValidSelectionTile_Implementation(
        AGridTileBase* Tile) const;

    // Called when cursor moves onto a valid tile
    // Subclass sends GameplayTag to tile to drive visuals
    UFUNCTION(BlueprintNativeEvent, Category = "Action|Selection")
    void HandleValidHover(AGridTileBase* Tile);
    virtual void HandleValidHover_Implementation(AGridTileBase* Tile);

    // Called when cursor leaves a previously valid tile
    // Subclass clears tile visual state via GameplayTag
    UFUNCTION(BlueprintNativeEvent, Category = "Action|Selection")
    void HandleHoverCleared(AGridTileBase* PreviousTile);
    virtual void HandleHoverCleared_Implementation(
        AGridTileBase* PreviousTile);

    // Called when player confirms a valid selection
    // Subclass builds FTurnActionRequest and fires OnChangeRequested
    UFUNCTION(BlueprintNativeEvent, Category = "Action|Selection")
    void HandleValidSelection(AGridTileBase* Tile);
    virtual void HandleValidSelection_Implementation(AGridTileBase* Tile);

    // Called on cancel or complete to clean up any selection state
    // Subclass clears any tile visual state it set
    UFUNCTION(BlueprintNativeEvent, Category = "Action|Selection")
    void ClearSelectionState();
    virtual void ClearSelectionState_Implementation();

    // --- Virtual Cooldown Hook ---

    // Controls whether cooldown ticks on this turn
    // Default: only ticks on owning participant's turn
    // Override for global cooldown behaviour
    UFUNCTION(BlueprintNativeEvent, Category = "Action|Cooldown")
    bool ShouldTickCooldown(bool bIsOwningParticipantTurn) const;
    virtual bool ShouldTickCooldown_Implementation(
        bool bIsOwningParticipantTurn) const;

    // --- Helper for subclasses ---

    // Convenience -- fires OnChangeRequested with a built request
    // Subclasses call this from HandleValidSelection_Implementation
    void RequestBoardChange(const FTurnActionRequest& Request);

    // Currently hovered tile -- tracked while action is active
    UPROPERTY()
    TObjectPtr<AGridTileBase> CurrentHoveredTile = nullptr;

    // Owning controller -- set by InitialiseAction
    UPROPERTY()
    TObjectPtr<AController> OwningController = nullptr;

    UGridWorldSubsystem* GetGridSubsystem() const;

private:

    // Input component -- set by InitialiseAction
    // Stored as enhanced input component for binding
    UPROPERTY()
    TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent = nullptr;

    // Binds to subsystem hover and Enhanced Input on Activate
    void BindSelectionInput();

    // Removes all bindings on Complete or Cancel
    void UnbindSelectionInput();

    UFUNCTION()
    void OnGridTileHoverChanged(AGridTileBase* NewTile);

    void OnSelectionInputTriggered();
};
