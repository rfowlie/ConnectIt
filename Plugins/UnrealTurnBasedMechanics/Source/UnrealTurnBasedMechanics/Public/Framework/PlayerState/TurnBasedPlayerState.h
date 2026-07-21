// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedPlayerState.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnsMissedChanged, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnForfeited);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReadyChanged, bool, bIsReady);

UCLASS()
class UNREALTURNBASEDMECHANICS_API ATurnBasedPlayerState : public APlayerState
{
    GENERATED_BODY()

    // Manager writes these -- nobody else should
    friend class UTurnBasedParticipantManagerComponent;

public:

    ATurnBasedPlayerState();

    // --- Queries ---

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    int32 GetSlotIndex() const { return SlotIndex; }

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    int32 GetTurnsMissed() const { return TurnsMissed; }

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    bool IsForfeited() const { return bForfeited; }

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    bool IsReady() const { return bIsReady; }

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    EParticipantType GetParticipantType() const { return ParticipantType; }

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnTurnsMissedChanged OnTurnsMissedChanged;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnForfeited OnForfeited;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnReadyChanged OnReadyChanged;

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

    // --- Manager-only setters (friend access) ---

    void SetSlotIndex(int32 InSlotIndex);
    void SetParticipantType(EParticipantType InType);
    void IncrementTurnsMissed();
    void SetForfeited(bool bInForfeited);
    void SetReady(bool bInReady);
    void ResetTurnState();

private:

    // Stable for lifetime of match -- doubles as FactionID
    UPROPERTY(Replicated)
    int32 SlotIndex = -1;

    UPROPERTY(Replicated)
    EParticipantType ParticipantType = EParticipantType::Human;

    UPROPERTY(ReplicatedUsing = OnRep_TurnsMissed)
    int32 TurnsMissed = 0;

    UPROPERTY(ReplicatedUsing = OnRep_Forfeited)
    bool bForfeited = false;

    UPROPERTY(ReplicatedUsing = OnRep_Ready)
    bool bIsReady = false;

    // --- RepNotify ---
    
    UFUNCTION()
    void OnRep_TurnsMissed();

    UFUNCTION()
    void OnRep_Forfeited();

    UFUNCTION()
    void OnRep_Ready();
};