// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "TurnBasedAIController.generated.h"

class UTurnBasedControllerCoordinatorComponent;
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

    // Takes FObjectInitializer (default-valued, so every existing no-arg call
    // site is unaffected) so a project-specific subclass can override
    // ActionsComponent's concrete class via
    // ObjectInitializer.SetDefaultSubobjectClass<T>(TEXT("ActionsComponent"))
    // before calling Super(ObjectInitializer) -- same technique, same reason,
    // as ATurnBasedPlayerControllerBase's own constructor.
    explicit ATurnBasedAIController(
        const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Components")
    TObjectPtr<UTurnBasedParticipantComponent> ParticipantComponent = nullptr;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Components")
    TObjectPtr<UTurnBasedActionsComponent> ActionsComponent = nullptr;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Components")
    TObjectPtr<UTurnBasedControllerCoordinatorComponent>
        CoordinatorComponent = nullptr;

    // Display name assigned to the AI PlayerState
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Config")
    FString AIDisplayName = TEXT("AI Opponent");

    // PlayerState created automatically in BeginPlay when true
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Config")
    bool bAutoCreatePlayerState = true;

protected:

    virtual void BeginPlay() override;

    // Creates and configures PlayerState required for participation
    // AI controllers do not create one by default
    UFUNCTION(BlueprintCallable, Category = "Turn Based")
    void EnsurePlayerState();

	
};
