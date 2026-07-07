// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Subsystem/ConnectItBlackboardSubsystem.h"

#include "TurnBasedMechanicsStructs.h"


void UConnectItBlackboardSubsystem::ApplyModifier(
	int32 TargetSlotIndex,
	const FTurnModifier& Modifier)
{
	PendingModifiers.Add(TargetSlotIndex, Modifier);
	OnModifierApplied.Broadcast(TargetSlotIndex, Modifier);

	UE_LOG(LogTemp, Log,
		TEXT("ConnectItBlackboard: Modifier %s applied to slot %d"),
		*UEnum::GetValueAsString(Modifier.ModifierType),
		TargetSlotIndex);
}

void UConnectItBlackboardSubsystem::ClearModifier(int32 TargetSlotIndex)
{
	if (PendingModifiers.Remove(TargetSlotIndex) > 0)
	{
		OnModifierCleared.Broadcast(TargetSlotIndex);

		UE_LOG(LogTemp, Log,
			TEXT("ConnectItBlackboard: Modifier cleared for slot %d"),
			TargetSlotIndex);
	}
}

bool UConnectItBlackboardSubsystem::HasModifier(int32 SlotIndex) const
{
	return PendingModifiers.Contains(SlotIndex);
}

FTurnModifier UConnectItBlackboardSubsystem::GetModifier(int32 SlotIndex) const
{
	const FTurnModifier* Found = PendingModifiers.Find(SlotIndex);
	return Found ? *Found : FTurnModifier();
}

TArray<int32> UConnectItBlackboardSubsystem::GetAllModifiedSlots() const
{
	TArray<int32> Keys;
	PendingModifiers.GenerateKeyArray(Keys);
	return Keys;
}
