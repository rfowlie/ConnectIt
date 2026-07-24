// Fill out your copyright notice in the Description page of Project Settings.

// TurnBasedGameState.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "TurnBasedMechanicsStructs.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedGameState.generated.h"

class UTurnBasedParticipantManagerComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchPhaseChanged, EMatchPhase, NewPhase);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMatchPhaseChanged_Native, EMatchPhase);

UCLASS()
class UNREALTURNBASEDMECHANICS_API ATurnBasedGameState : public AGameState
{
    GENERATED_BODY()

    // Manager writes match phase -- nobody else should
    friend class UTurnBasedParticipantManagerComponent;

public:

    ATurnBasedGameState();

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Turn Based")
    TObjectPtr<UTurnBasedParticipantManagerComponent> ParticipantManager = nullptr;

    // --- Match Phase ---

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    EMatchPhase GetMatchPhase() const { return MatchPhase; }

    // All clients bind here -- fires on server immediately
    // and on clients via OnRep
    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnMatchPhaseChanged OnMatchPhaseChanged;

    FOnMatchPhaseChanged_Native OnMatchPhaseChanged_Native;

    // --- Passthrough Accessors ---
    // Read from participant manager -- no duplicated state

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    ETurnPhase GetCurrentTurnPhase() const;

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    int32 GetActiveParticipantIndex() const;

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    int32 GetActiveTurnNumber() const;

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    const TArray<FTurnParticipantInfo>& GetParticipants() const;

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    float GetTurnDuration() const;

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

    // Manager-only setter -- friend access
    void SetMatchPhase(EMatchPhase NewPhase);

private:

    UPROPERTY(ReplicatedUsing = OnRep_MatchPhase)
    EMatchPhase MatchPhase = EMatchPhase::WaitingForParticipants;

    UFUNCTION()
    void OnRep_MatchPhase();

    void BroadcastMatchPhaseChanged();
};
