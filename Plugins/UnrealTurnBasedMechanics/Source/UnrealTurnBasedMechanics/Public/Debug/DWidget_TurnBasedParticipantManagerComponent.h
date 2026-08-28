// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widget/DWidgetBase.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedMechanicsStructs.h"
#include "DWidget_TurnBasedParticipantManagerComponent.generated.h"

class UTurnBasedParticipantManagerComponent;

// Tracks UTurnBasedParticipantManagerComponent specifically -- turn phase,
// active participant index, turn number, and the participant list, pushed
// straight through to BP as each change (see UDWidgetBase's own class
// comment for the push/GetInfo() convention). Kept as its own widget
// (rather than folded into a game-state widget) so each widget's update
// footprint maps to exactly one class's delegates -- see the consuming
// project's debug-widget workflow doc for the full rationale and the
// complete widget<->class mapping.
//
// OnActiveControllerChanged hands out AController*, not the index this
// widget actually wants -- the handler reads ResolvedSource
// ::ActiveParticipantIndex directly instead (already updated by the time
// this fires) rather than leaking the controller pointer into BP.
UCLASS(Abstract)
class UNREALTURNBASEDMECHANICS_API UDWidget_TurnBasedParticipantManagerComponent : public UDWidgetBase
{
    GENERATED_BODY()

public:

    // False until UTurnBasedParticipantManagerComponent has actually been
    // resolved -- the one deliberately pull-only exception to the push
    // model, checked once rather than pushed.
    UFUNCTION(BlueprintPure, Category = "Debug")
    bool IsSourceValid() const { return bSourceValid; }

protected:

    virtual void BindDelegates() override;
    virtual void UnbindDelegates() override;

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Debug")
    void OnTurnPhaseUpdated(ETurnPhase NewPhase);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Debug")
    void OnActiveParticipantIndexUpdated(int32 NewIndex);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Debug")
    void OnTurnNumberUpdated(int32 NewTurnNumber);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Debug")
    void OnParticipantsUpdated(const TArray<FTurnParticipantInfo>& NewParticipants);

private:

    UFUNCTION()
    void HandleTurnPhaseChanged(ETurnPhase NewPhase);

    UFUNCTION()
    void HandleActiveControllerChanged(AController* NewActiveController);

    UFUNCTION()
    void HandleParticipantForfeited(const FTurnParticipantInfo& ParticipantInfo);

    // Zero-param pings with no single field they map to -- re-derive and
    // push everything via GetInfo(), same fallback shape the zero-param
    // board-state/game-event-subsystem cases already use.
    UFUNCTION()
    void HandleAllParticipantsReady();

    UFUNCTION()
    void HandleGameOver();

    void PushCurrentInfo();

    UPROPERTY()
    TObjectPtr<UTurnBasedParticipantManagerComponent> ResolvedSource = nullptr;

    bool bSourceValid = false;
};
