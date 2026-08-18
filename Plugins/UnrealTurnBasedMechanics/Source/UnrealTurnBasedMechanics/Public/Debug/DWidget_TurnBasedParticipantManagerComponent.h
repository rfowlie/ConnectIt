// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widget/DWidgetBase.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedMechanicsStructs.h"
#include "DWidget_TurnBasedParticipantManagerComponent.generated.h"

class UTurnBasedParticipantManagerComponent;

// Tracks UTurnBasedParticipantManagerComponent specifically -- turn phase,
// active participant, turn number, and the participant list. Kept as its
// own widget (rather than folded into a game-state widget) so each widget's
// refresh footprint maps to exactly one class's delegates -- see the
// consuming project's debug-widget workflow doc for the full rationale and
// the complete widget<->class mapping.
//
// GetTurnNumber() benefits directly from TurnNumber's ReplicatedUsing =
// OnRep_TurnNumber -- OnTurnPhaseChanged is guaranteed to fire on every
// turn advance, so this widget needs no fallback signal.
UCLASS(Abstract)
class UNREALTURNBASEDMECHANICS_API UDWidget_TurnBasedParticipantManagerComponent : public UDWidgetBase
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintPure, Category = "Debug")
    ETurnPhase GetTurnPhase() const { return CachedTurnPhase; }

    UFUNCTION(BlueprintPure, Category = "Debug")
    int32 GetActiveParticipantIndex() const { return CachedActiveParticipantIndex; }

    UFUNCTION(BlueprintPure, Category = "Debug")
    int32 GetTurnNumber() const { return CachedTurnNumber; }

    UFUNCTION(BlueprintPure, Category = "Debug")
    const TArray<FTurnParticipantInfo>& GetParticipants() const { return CachedParticipants; }

    // False until UTurnBasedParticipantManagerComponent has actually been resolved
    UFUNCTION(BlueprintPure, Category = "Debug")
    bool IsSourceValid() const { return bSourceValid; }

protected:

    virtual void BindDelegates() override;
    virtual void UnbindDelegates() override;
    virtual void RefreshFields() override;

private:

    UFUNCTION()
    void HandleTurnPhaseChanged(ETurnPhase NewPhase);

    UFUNCTION()
    void HandleActiveControllerChanged(AController* NewActiveController);

    UFUNCTION()
    void HandleParticipantForfeited(const FTurnParticipantInfo& ParticipantInfo);

    UFUNCTION()
    void HandleAllParticipantsReady();

    UFUNCTION()
    void HandleGameOver();

    UPROPERTY()
    TObjectPtr<UTurnBasedParticipantManagerComponent> ResolvedSource = nullptr;

    ETurnPhase CachedTurnPhase = ETurnPhase::WaitingForParticipants;
    int32 CachedActiveParticipantIndex = INDEX_NONE;
    int32 CachedTurnNumber = 0;
    TArray<FTurnParticipantInfo> CachedParticipants;
    bool bSourceValid = false;
};
