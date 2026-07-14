// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TurnBasedMechanicsStructs.h"
#include "Subsystems/WorldSubsystem.h"
#include "ConnectIt_BlackboardSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnModifierApplied, int32, TargetSlotIndex, const FTurnModifier&, Modifier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnModifierCleared, int32, TargetSlotIndex);

UCLASS()
class CONNECTIT_API UConnectIt_BlackboardSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	// --- Modifier API ---

	// Write a modifier targeting a specific participant slot
	// Called by player actions (shard activations etc.)
	UFUNCTION(BlueprintCallable, Category = "ConnectIt|Blackboard")
	void ApplyModifier(int32 TargetSlotIndex, const FTurnModifier& Modifier);

	// Clear modifier for a slot — called by AI after consuming
	UFUNCTION(BlueprintCallable, Category = "ConnectIt|Blackboard")
	void ClearModifier(int32 TargetSlotIndex);

	// Query API — called by AI controller on turn start
	UFUNCTION(BlueprintPure, Category = "ConnectIt|Blackboard")
	bool HasModifier(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "ConnectIt|Blackboard")
	FTurnModifier GetModifier(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "ConnectIt|Blackboard")
	TArray<int32> GetAllModifiedSlots() const;

	// --- Delegates ---

	// Fired when a modifier is applied — AI or UI can react
	UPROPERTY(BlueprintAssignable, Category = "ConnectIt|Blackboard")
	FOnModifierApplied OnModifierApplied;

	UPROPERTY(BlueprintAssignable, Category = "ConnectIt|Blackboard")
	FOnModifierCleared OnModifierCleared;

private:

	// Keyed by participant SlotIndex
	TMap<int32, FTurnModifier> PendingModifiers;
};
