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

    // Passthroughs to ParticipantManager's own queries of the same name --
    // see their doc comments there.
    UFUNCTION(BlueprintPure, Category = "Turn Based")
    FTurnParticipantInfo GetActiveParticipant(bool& bOutValid) const;

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    FTurnParticipantInfo GetParticipantBySlot(int32 InSlotIndex, bool& bOutValid) const;

    // Seconds left on the active participant's turn, derived locally from
    // ParticipantManager's replicated turn-start server timestamp + turn
    // duration -- no per-tick replication and no Client RPC. Unlike
    // FTurnNotification::TurnDuration (which only reaches the active
    // participant via ClientReceiveTurnNotification), any client can call
    // this for the active participant's turn. bOutTimerRunning is false
    // (return 0) when no timer is currently active (between turns, paused,
    // game over).
    UFUNCTION(BlueprintPure, Category = "Turn Based")
    float GetTurnTimeRemaining(bool& bOutTimerRunning) const;

    // 0..1 for a radial/bar widget. 0 when no timer is running.
    UFUNCTION(BlueprintPure, Category = "Turn Based")
    float GetTurnTimeRemainingFraction() const;

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
