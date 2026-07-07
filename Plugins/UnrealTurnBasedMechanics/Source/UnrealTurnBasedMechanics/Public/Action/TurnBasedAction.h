// Fill out your copyright notice in the Description page of Project Settings.

// TurnBasedAction.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedAction.generated.h"

class AGridTileBase;
class UGridWorldSubsystem;
class UInputComponent;
class UEnhancedInputComponent;
class UInputAction;
class UTurnBasedAction;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnBasedActionEvent, UTurnBasedAction*, Action);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnBasedActionEvent_Native, UTurnBasedAction*);

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class UNREALTURNBASEDMECHANICS_API UTurnBasedAction : public UObject
{
    GENERATED_BODY()

public:

    // --- Designer Config — set in DataAsset ---

    // Tag identifying this action type
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    FGameplayTag ActionTag;

    // If true the turn cannot end without this action completing
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    bool bIsRequired = false;

    // If false the action cannot be cancelled once started
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    bool bIsCancellable = true;

    // If true this action waits for a grid tile selection before completing
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    bool bRequiresSelection = false;

    // How many times this action can be used per turn — 0 = unlimited
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config",
        meta = (ClampMin = 0))
    int32 MaxUsesPerTurn = 1;

    // Turns before this action can be used again after use — 0 = no cooldown
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config",
        meta = (ClampMin = 0))
    int32 CooldownTurns = 0;

    // Input action used to confirm selection — set per project
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

    // Called by the action component to initialise the action
    // Provides world context references
    void InitialiseAction(
        AController* InOwningController,
        UInputComponent* InInputComponent);

    // Called when the player activates this action
    UFUNCTION(BlueprintCallable, Category = "Action")
    void Activate();

    // Called to cancel the active action
    UFUNCTION(BlueprintCallable, Category = "Action")
    void Cancel();

    // Called internally when the action finishes successfully
    UFUNCTION(BlueprintCallable, Category = "Action")
    void Complete();

    // Whether this action can currently be activated
    UFUNCTION(BlueprintPure, Category = "Action")
    bool CanActivate() const;

    bool IsComplete() const
    {
        return State == ETurnBasedActionState::Completed;
    }

    // Called by action component on each turn start
    // bIsOwningParticipantTurn — whether this is the owning controller's turn
    UFUNCTION(BlueprintCallable, Category = "Action")
    void TickCooldown(bool bIsOwningParticipantTurn);

    // Reset per-turn state — called at start of each owning turn
    void ResetTurnState();

    // --- Delegates ---

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

    // Override to define which tiles are valid to hover
    UFUNCTION(BlueprintNativeEvent, Category = "Action|Selection")
    bool IsValidHoverTile(AGridTileBase* Tile) const;
    virtual bool IsValidHoverTile_Implementation(AGridTileBase* Tile) const;

    // Override to define which tiles are valid to select
    UFUNCTION(BlueprintNativeEvent, Category = "Action|Selection")
    bool IsValidSelectionTile(AGridTileBase* Tile) const;
    virtual bool IsValidSelectionTile_Implementation(AGridTileBase* Tile) const;

    // Override to drive tile visuals on valid hover
    // Use FGameplayTag to communicate state to tile Blueprint
    UFUNCTION(BlueprintNativeEvent, Category = "Action|Selection")
    void HandleValidHover(AGridTileBase* Tile);
    virtual void HandleValidHover_Implementation(AGridTileBase* Tile);

    // Override to clear hover visuals
    UFUNCTION(BlueprintNativeEvent, Category = "Action|Selection")
    void HandleHoverCleared(AGridTileBase* PreviousTile);
    virtual void HandleHoverCleared_Implementation(AGridTileBase* PreviousTile);

    // Override to handle a valid tile selection
    UFUNCTION(BlueprintNativeEvent, Category = "Action|Selection")
    void HandleValidSelection(AGridTileBase* Tile);
    virtual void HandleValidSelection_Implementation(AGridTileBase* Tile);

    // Override to clean up any spawned actors or visual state
    UFUNCTION(BlueprintNativeEvent, Category = "Action|Selection")
    void ClearVisuals();
    virtual void ClearVisuals_Implementation();

    // Override to control cooldown tick behaviour
    // Default — ticks only on owning participant turn
    UFUNCTION(BlueprintNativeEvent, Category = "Action|Cooldown")
    bool ShouldTickCooldown(bool bIsOwningParticipantTurn) const;
    virtual bool ShouldTickCooldown_Implementation(
        bool bIsOwningParticipantTurn) const;

    // Cached references — set by InitialiseAction
    UPROPERTY()
    TObjectPtr<AController> OwningController = nullptr;

    UPROPERTY()
    TObjectPtr<UInputComponent> InputComponent = nullptr;

    // Currently hovered tile — tracked while action is active
    UPROPERTY()
    TObjectPtr<AGridTileBase> CurrentHoveredTile = nullptr;

    // Subsystem shortcut
    UGridWorldSubsystem* GetGridSubsystem() const;

private:

    // Binds to UGridWorldSubsystem hover broadcast
    void BindSelectionInput();
    void UnbindSelectionInput();

    // Handles hover change from subsystem
    UFUNCTION()
    void OnGridTileHoverChanged(AGridTileBase* NewTile);

    // Handles selection input from Enhanced Input
    void OnSelectionInputTriggered();

    // Enhanced input component handle for cleanup
    int32 SelectionInputBindingHandle = -1;
};