// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TurnBasedMechanicsStructs.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedControllerCoordinatorComponent.generated.h"

class UTurnBasedParticipantComponent;
class UTurnBasedActionsComponent;
class ATurnBasedGameState;

// Mediator between UTurnBasedParticipantComponent and
// UTurnBasedActionsComponent
//
// Exists because APlayerController and AAIController are sibling
// branches in the engine hierarchy -- neither can inherit shared
// wiring from the other
//
// Both controller types add this component and get identical
// turn-to-action routing with zero duplicated code
//
// Neither participant nor actions component knows the other exists
UCLASS(ClassGroup=(TurnBased), meta=(BlueprintSpawnableComponent))
class UNREALTURNBASEDMECHANICS_API UTurnBasedControllerCoordinatorComponent
    : public UActorComponent
{
    GENERATED_BODY()

public:

    UTurnBasedControllerCoordinatorComponent();

    // --- Config ---

    // When true the coordinator binds to game state match phase changes
    // and routes them to the actions component
    // Disable if a project handles match phase routing itself
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Turn Based|Coordinator")
    bool bRouteMatchPhaseChanges = true;

    // --- Queries ---

    UFUNCTION(BlueprintPure, Category = "Turn Based|Coordinator")
    UTurnBasedParticipantComponent* GetParticipantComponent() const
    {
        return ParticipantComponent;
    }

    UFUNCTION(BlueprintPure, Category = "Turn Based|Coordinator")
    UTurnBasedActionsComponent* GetActionsComponent() const
    {
        return ActionsComponent;
    }

    UFUNCTION(BlueprintPure, Category = "Turn Based|Coordinator")
    bool IsWired() const { return bIsWired; }

    // Manually trigger wiring if components were added after BeginPlay
    UFUNCTION(BlueprintCallable, Category = "Turn Based|Coordinator")
    void WireComponents();

protected:

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

    // Resolved on BeginPlay from owning controller
    UPROPERTY()
    TObjectPtr<UTurnBasedParticipantComponent> ParticipantComponent = nullptr;

    UPROPERTY()
    TObjectPtr<UTurnBasedActionsComponent> ActionsComponent = nullptr;

    bool bIsWired  = false;
    bool bIsPaused = false;

    // --- Wiring ---

    bool ResolveComponents();
    void BindParticipantToActions();
    void BindActionsToParticipant();
    void BindGameStateToActions();
    void UnbindAll();

    // --- Participant -> Actions ---

    UFUNCTION()
    void HandleTurnNotification(const FTurnNotification& Notification);

    UFUNCTION()
    void HandleOpponentTurnStarted(int32 ActiveParticipantSlotIndex);

    // --- Actions -> Participant ---

    UFUNCTION()
    void HandleTurnEndRequested();

    // --- GameState -> Actions ---

    UFUNCTION()
    void HandleMatchPhaseChanged(EMatchPhase NewPhase);

    // --- Helpers ---

    ATurnBasedGameState* GetTurnBasedGameState() const;
};