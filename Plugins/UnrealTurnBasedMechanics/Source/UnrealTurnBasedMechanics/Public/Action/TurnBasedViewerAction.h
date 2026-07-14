// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "TurnBasedViewerAction.generated.h"

class UTurnBasedViewerAction;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnViewerActionStateChanged, UTurnBasedViewerAction*, ViewerAction);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnViewerActionStateChanged_Native, UTurnBasedViewerAction*);

// Base class for passive controller actions that run when it
// is not this participant's turn
//
// Key differences from UTurnBasedAction:
//   -- Never completes naturally -- deactivated by external signal
//   -- Cannot request board state changes
//   -- Not queued or managed by required action system
//   -- Single instance per controller -- no cooldown or uses tracking
//   -- World access via typed outer (same pattern as UTurnBasedAction)
UCLASS(Abstract, Blueprintable, BlueprintType,
    EditInlineNew, DefaultToInstanced)
class UNREALTURNBASEDMECHANICS_API UTurnBasedViewerAction : public UObject
{
    GENERATED_BODY()

public:

    // Tag identifying this viewer action type -- for debugging and logging
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Viewer Action")
    FGameplayTag ActionTag;

    // --- Lifecycle ---

    // Called by component when opponent's turn starts
    // or on match start before first turn
    void Activate(AController* InOwningController);

    // Called by component when this participant's turn starts
    // or when match ends
    // NOT called by the action itself -- always external
    void Deactivate();

    UFUNCTION(BlueprintPure, Category = "Viewer Action")
    bool IsActive() const { return bIsActive; }

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Viewer Action")
    FOnViewerActionStateChanged OnActionActivated;

    UPROPERTY(BlueprintAssignable, Category = "Viewer Action")
    FOnViewerActionStateChanged OnActionDeactivated;

    FOnViewerActionStateChanged_Native OnActivated_Native;
    FOnViewerActionStateChanged_Native OnDeactivated_Native;

    // --- World Access ---

    virtual UWorld* GetWorld() const override;

protected:

    // Override to set up passive input, camera binding,
    // board state observation etc.
    UFUNCTION(BlueprintNativeEvent, Category = "Viewer Action")
    void OnActivated();

    // Override to clean up anything set up in OnActivated
    // Must be safe to call multiple times
    UFUNCTION(BlueprintNativeEvent, Category = "Viewer Action")
    void OnDeactivated();

    // Owning controller -- set on Activate, cleared on Deactivate
    UPROPERTY(BlueprintReadOnly, Category = "Viewer Action")
    TObjectPtr<AController> OwningController = nullptr;

private:

    bool bIsActive = false;
};
