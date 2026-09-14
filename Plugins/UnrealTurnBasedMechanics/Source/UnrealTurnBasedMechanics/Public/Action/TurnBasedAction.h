// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InputTriggers.h"
#include "Styling/SlateBrush.h"
#include "TurnBasedMechanicsStructs.h"
#include "Action/TurnBasedActionBase.h"
#include "Input/InputTagBinder.h"
#include "TurnBasedAction.generated.h"

class UTurnBasedAction;
class AGridTileBase;
class UGridHoverSubsystem;
class UEnhancedInputComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;


// Fired when this action wants to chain into another action,
// Action component listens to this and calls TryPushAction(Class)
// Example: a menu action fires this when the player selects an option
// Example: an ability selection action fires this to push targeting action
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNextActionRequested, TSubclassOf<UTurnBasedAction>, NextActionClass);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNextActionRequested_Native, TSubclassOf<UTurnBasedAction>);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnActionEvent, UTurnBasedAction*, Action);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnActionRequested, const FTurnActionRequest&, Request);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnBasedActionEvent_Native, UTurnBasedAction*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnActionRequested_Native, const FTurnActionRequest&);


UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class UNREALTURNBASEDMECHANICS_API UTurnBasedAction : public UTurnBasedActionBase
{
    GENERATED_BODY()

public:

    // --- Designer Config ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    bool bIsRequired = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    bool bIsCancellable = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    bool bRequiresSelection = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    bool bAllowsOptionalInterrupt = true;

    // Maximum completions per turn -- 0 = unlimited
    // Checked against CompletionsThisTurn on CanActivate
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config",
        meta = (ClampMin = 0))
    int32 MaxCompletionsPerTurn = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config",
        meta = (ClampMin = 0))
    int32 CooldownTurns = 0;

    // TODO: no need for a default input action, each action will be super customized and actions will be set
    // UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    // TObjectPtr<UInputAction> SelectionInputAction = nullptr;
    //
    // // Key mapped to SelectionInputAction on InputTagBinder's mapping
    // // context (see InitialiseAction) -- SelectionInputAction has no
    // // InputBindings entry of its own, so it needs its own key here.
    // UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    // FKey SelectionInputKey;

    // Additional named input triggers beyond SelectionInputAction -- see
    // FInputTagBinding (UnrealGameMechanics/Input). Bound/unbound alongside
    // SelectionInputAction by BindInput/UnbindInput, dispatched through
    // InputTagBinder.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    TArray<FInputTagBinding> InputBindings;

    // --- Presentation ---
    // Purely for UI -- the action itself never reads these. Without them an
    // action bar has nothing to label a button with (ActionTag is an
    // identifier, not a display string).

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Presentation")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Presentation",
        meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Presentation")
    FSlateBrush Icon;

    // --- Runtime State ---

    // Only incremented on Complete() -- not on Activate or Cancel
    // Reset at start of each turn
    UPROPERTY(BlueprintReadOnly, Category = "Action|State")
    int32 CompletionsThisTurn = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Action|State")
    int32 TurnsUntilAvailable = 0;

    // --- Lifecycle ---

    void InitialiseAction(
        AController* InOwningController,
        UEnhancedInputComponent* InInputComponent,
        UEnhancedInputLocalPlayerSubsystem* InLocalPlayerSubsystem);

    // Natural completion -- increments CompletionsThisTurn
    UFUNCTION(BlueprintCallable, Category = "Action")
    void Complete();

    // Player cancel -- no CompletionsThisTurn increment
    UFUNCTION(BlueprintCallable, Category = "Action")
    void Cancel();

    // System interrupt -- inherited from base, no increment
    // ForceDeactivate() also unbinds selection input

    UFUNCTION(BlueprintPure, Category = "Action")
    bool CanActivate() const;

    UFUNCTION(BlueprintPure, Category = "Action")
    bool IsComplete() const
    {
        return CompletionsThisTurn > 0;
    }

    void TickCooldown(bool bIsMyTurn);

    // Resets CompletionsThisTurn -- called at turn start
    void ResetTurnState();

    // --- Board Change ---

    UPROPERTY(BlueprintAssignable, Category = "Action|Delegates")
    FOnTurnActionRequested OnChangeRequested;

    FOnTurnActionRequested_Native OnChangeRequested_Native;

    // --- Action Events ---
    // Separate from base OnActivated/OnDeactivated
    // Carry UTurnBasedAction* for typed access

    UPROPERTY(BlueprintAssignable, Category = "Action|Delegates")
    FOnTurnActionEvent OnActionCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Action|Delegates")
    FOnTurnActionEvent OnActionCancelled;

    FOnTurnBasedActionEvent_Native OnActionCompleted_Native;
    FOnTurnBasedActionEvent_Native OnActionCancelled_Native;
    
    UPROPERTY(BlueprintAssignable, Category = "Action|Delegates")
    FOnNextActionRequested OnNextActionRequested;

    FOnNextActionRequested_Native OnNextActionRequested_Native;

    // Helper -- subclasses call this to request chaining
    // Component receives and pushes the named action onto the stack
    UFUNCTION(BlueprintCallable, Category = "Action")
    void RequestNextAction(TSubclassOf<UTurnBasedAction> NextActionClass);
    
protected:

    // --- Virtual Lifecycle Hooks ---

    virtual void Activate_Internal_Implementation() override;
    virtual void Deactivate_Internal_Implementation() override;
    virtual void ForceDeactivate_Internal_Implementation() override;

    UFUNCTION(BlueprintNativeEvent, Category = "Action")
    void OnCancelled();

    UFUNCTION(BlueprintNativeEvent, Category = "Action")
    void OnCompleted();

    // --- Selection Hooks ---

    UFUNCTION(BlueprintNativeEvent, Category = "Action|Selection")
    bool IsValidHoverTile(AGridTileBase* Tile) const;
    virtual bool IsValidHoverTile_Implementation(AGridTileBase* Tile) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action|Selection")
    bool IsValidSelectionTile(AGridTileBase* Tile) const;
    virtual bool IsValidSelectionTile_Implementation(AGridTileBase* Tile) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action|Selection")
    void HandleValidHover(AGridTileBase* Tile);
    virtual void HandleValidHover_Implementation(AGridTileBase* Tile);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action|Selection")
    void HandleHoverCleared(AGridTileBase* PreviousTile);
    virtual void HandleHoverCleared_Implementation(AGridTileBase* PreviousTile);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action|Selection")
    void HandleValidSelection(AGridTileBase* Tile);
    virtual void HandleValidSelection_Implementation(AGridTileBase* Tile);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action|Selection")
    void ClearSelectionState();
    virtual void ClearSelectionState_Implementation();

    // --- Bound Input ---

    // Fires when any InputBindings entry triggers, carrying that entry's
    // BindingTag. Default implementation is a no-op -- subclasses (usually
    // in Blueprint) switch on BindingTag to route to actual behaviour.
    // SelectionInputAction/OnSelectionInputTriggered stays separate since
    // it's tied to CurrentHoveredTile, not a generic named trigger.
    UFUNCTION(BlueprintNativeEvent, Category = "Action|Input")
    void OnBoundInputTriggered(FGameplayTag BindingTag);
    virtual void OnBoundInputTriggered_Implementation(FGameplayTag BindingTag);

    // --- Cooldown Hook ---

    UFUNCTION(BlueprintNativeEvent, Category = "Action|Cooldown")
    bool ShouldTickCooldown(bool bIsMyTurn) const;
    virtual bool ShouldTickCooldown_Implementation(bool bIsMyTurn) const;

    // Fires OnChangeRequested -- subclasses call from HandleValidSelection
    void RequestBoardChange(const FTurnActionRequest& Request);

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<AGridTileBase> CurrentHoveredTile = nullptr;

    UGridHoverSubsystem* GetGridSubsystem() const;
    
    // Bind/unbind grid-tile hover + optional Enhanced Input selection.
    // Called automatically from Activate_Internal_Implementation when
    // bRequiresSelection is true. Exposed to subclasses so an action that
    // defers selection to a later internal state (bRequiresSelection left
    // false) can call these directly once it's ready -- see
    // ConnectIt_TutorialShiftIntroAction for an example.
    void BindInput();
    void UnbindInput();

    void BindGridSubsystem();
    void UnbindGridSubsystem();

    // --- Action Helpers ---
    // TODO: create helper functions for default action items (grid selection, piece selection, etc.)
    // default function for activating selection
    UFUNCTION(BlueprintCallable)
    void OnSelectionInputTriggered();

private:

    UPROPERTY()
    TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent = nullptr;

    UPROPERTY()
    TObjectPtr<UEnhancedInputLocalPlayerSubsystem> LocalPlayerSubsystem = nullptr;

    // Owns InputBindings' mapping context + dispatch -- see
    // UnrealGameMechanics/Input/InputTagBinder.h. SelectionInputAction
    // piggybacks one extra key mapping onto this same context (see
    // InitialiseAction) rather than standing up a second one.
    UPROPERTY()
    TObjectPtr<UInputTagBinder> InputTagBinder = nullptr;

    // Shared tail of Complete()/Cancel() -- runs Deactivate_Internal,
    // clears the active flag, and broadcasts OnDeactivated
    void FinishAction();

    UFUNCTION()
    void OnGridTileHoverChanged(AGridTileBase* NewTile);

    // Bound to InputTagBinder->OnInputTagTriggered -- forwards to the
    // BlueprintNativeEvent hook
    UFUNCTION()
    void HandleInputTagTriggered(const FInputActionInstance& InputActionInstance, FGameplayTag BindingTag);
};