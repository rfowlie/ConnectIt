// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "TurnBasedAIController.generated.h"

class UTurnBasedParticipantComponent;
class UTurnBasedActionsComponent;


// Base AI controller for turn-based participation
// Handles PlayerState creation which is required for participant
// registration -- AI controllers do not create one by default
//
// Subclasses override the turn handlers to implement game intelligence
// (MinMax, behaviour trees, utility AI, scripted, etc.)
UCLASS(Abstract, Blueprintable, BlueprintType)
class UNREALTURNBASEDMECHANICS_API ATurnBasedAIController : public AAIController
{
	GENERATED_BODY()

public:
    
    ATurnBasedAIController();

    // --- Components ---

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Components")
    TObjectPtr<UTurnBasedParticipantComponent> ParticipantComponent = nullptr;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Components")
    TObjectPtr<UTurnBasedActionsComponent> ActionsComponent = nullptr;

    // --- Config ---

    // Display name assigned to the AI PlayerState
    // Override in Blueprint or subclass for per-opponent naming
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Config")
    FString AIDisplayName = TEXT("AI Opponent");

    // If true PlayerState is created automatically in BeginPlay
    // Disable only if a subclass handles creation itself
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Config")
    bool bAutoCreatePlayerState = true;

protected:

    virtual void BeginPlay() override;

    // Creates and configures the PlayerState required for participation
    // AI controllers do not create one by default -- this fills that gap
    // Called automatically in BeginPlay when bAutoCreatePlayerState is true
    // Safe to call manually -- does nothing if PlayerState already exists
    UFUNCTION(BlueprintCallable, Category = "Turn Based")
    void EnsurePlayerState();

    // --- Turn Handlers ---
    // Override these in subclasses to implement AI behaviour

    // Fired when it is this AI's turn
    // Subclass runs its decision making here
    UFUNCTION(BlueprintNativeEvent, Category = "Turn Based")
    void HandleTurnNotification(const FTurnNotification& Notification);
    virtual void HandleTurnNotification_Implementation(
        const FTurnNotification& Notification);

    // Fired when it is another participant's turn
    // Default notifies actions component -- override to add observation logic
    UFUNCTION(BlueprintNativeEvent, Category = "Turn Based")
    void HandleOpponentTurnStarted(int32 ActiveParticipantSlotIndex);
    virtual void HandleOpponentTurnStarted_Implementation(
        int32 ActiveParticipantSlotIndex);

private:

    // Binds participant component delegates -- called in BeginPlay
    void BindParticipantDelegates();

    UFUNCTION()
    void OnTurnNotificationReceived(const FTurnNotification& Notification);

    UFUNCTION()
    void OnOpponentTurnStartedReceived(int32 ActiveParticipantSlotIndex);

	
};
