// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "TurnBasedActionBase.generated.h"

class UTurnBasedActionBase;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionBaseEvent, UTurnBasedActionBase*, Action);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnActionBaseEvent_Native, UTurnBasedActionBase*);

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class UNREALTURNBASEDMECHANICS_API UTurnBasedActionBase : public UObject
{
    GENERATED_BODY()

public:

    // Tag identifying this action -- for debugging and logging
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
    FGameplayTag ActionTag;

    // --- Lifecycle ---

    // Called by action component when this action becomes active
    void Activate(AController* InOwningController);

    // Called by system events -- no use consumed, no completion logic
    // Safe to call even if not currently active
    void ForceDeactivate();

    UFUNCTION(BlueprintPure, Category = "Action")
    bool IsActive() const { return bIsActive; }

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Action|Delegates")
    FOnActionBaseEvent OnActivated;

    UPROPERTY(BlueprintAssignable, Category = "Action|Delegates")
    FOnActionBaseEvent OnDeactivated;

    // Fired when system force-deactivates this action
    // UI and external systems can respond to clean up
    UPROPERTY(BlueprintAssignable, Category = "Action|Delegates")
    FOnActionBaseEvent OnForceDeactivated;

    FOnActionBaseEvent_Native OnActivated_Native;
    FOnActionBaseEvent_Native OnDeactivated_Native;
    FOnActionBaseEvent_Native OnForceDeactivated_Native;

    // --- World Access ---

    virtual UWorld* GetWorld() const override;

    // Convenience -- returns null for AI controllers
    UFUNCTION(BlueprintPure, Category = "Action")
    APlayerController* GetPlayerController() const;

protected:

    void SetIsActive(const bool InActive) { bIsActive = InActive; }
    
    // Override to implement activation behaviour
    UFUNCTION(BlueprintNativeEvent, Category = "Action")
    void Activate_Internal();

    // Override to implement deactivation / cleanup
    UFUNCTION(BlueprintNativeEvent, Category = "Action")
    void Deactivate_Internal();

    // Override to implement force deactivation cleanup
    // Default calls OnDeactivated_Implementation
    // Override if force stop needs different behaviour than normal deactivate
    UFUNCTION(BlueprintNativeEvent, Category = "Action")
    void ForceDeactivate_Internal();

    UPROPERTY(BlueprintReadOnly, Category = "Action")
    TObjectPtr<AController> OwningController = nullptr;

private:

    bool bIsActive = false;
};