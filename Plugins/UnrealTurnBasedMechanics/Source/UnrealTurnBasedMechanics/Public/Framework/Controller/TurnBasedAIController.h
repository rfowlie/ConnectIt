// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "TurnBasedAIController.generated.h"

class UTurnBasedParticipantComponent;


// Base AI controller for turn-based participation
// Handles PlayerState creation which is required for participant
// registration -- AI controllers do not create one by default
//
// Deliberately has no actions component / coordinator: those exist for a
// human's input-driven action stack. An AI subclass handles its own turn
// directly -- bind ParticipantComponent->OnTurnNotificationReceived_Native
// for turn start/end, check ATurnBasedPlayerState::CanEndTurn, and call
// ParticipantComponent->ServerSubmitTurnEnd. AI participants are marked
// ready by the participant manager, so no ready-up is needed.
UCLASS(Abstract, Blueprintable, BlueprintType)
class UNREALTURNBASEDMECHANICS_API ATurnBasedAIController : public AAIController
{
	GENERATED_BODY()

public:

    // Takes FObjectInitializer (default-valued) so a subclass can override a
    // default subobject's class via SetDefaultSubobjectClass, the same way
    // ATurnBasedPlayerControllerBase's constructor allows.
    explicit ATurnBasedAIController(
        const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Components")
    TObjectPtr<UTurnBasedParticipantComponent> ParticipantComponent = nullptr;

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
