// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "Net/UnrealNetwork.h"


ATurnBasedPlayerState::ATurnBasedPlayerState()
{
	bReplicates = true;
}

void ATurnBasedPlayerState::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATurnBasedPlayerState, SlotIndex);
	DOREPLIFETIME(ATurnBasedPlayerState, ParticipantType);
	DOREPLIFETIME(ATurnBasedPlayerState, TurnsMissed);
	DOREPLIFETIME(ATurnBasedPlayerState, bForfeited);
	DOREPLIFETIME(ATurnBasedPlayerState, bIsReady);
}

// --- Manager-only setters ---

void ATurnBasedPlayerState::SetSlotIndex(int32 InSlotIndex)
{
	if (!HasAuthority()) return;
	SlotIndex = InSlotIndex;
}

void ATurnBasedPlayerState::SetParticipantType(EParticipantType InType)
{
	if (!HasAuthority()) return;
	ParticipantType = InType;
}

void ATurnBasedPlayerState::IncrementTurnsMissed()
{
	if (!HasAuthority()) return;

	TurnsMissed++;
	OnTurnsMissedChanged.Broadcast(TurnsMissed);
}

void ATurnBasedPlayerState::SetForfeited(bool bInForfeited)
{
	if (!HasAuthority()) return;
	if (bForfeited == bInForfeited) return;

	bForfeited = bInForfeited;
	if (bForfeited) OnForfeited.Broadcast();
}

void ATurnBasedPlayerState::SetReady(bool bInReady)
{
	if (!HasAuthority()) return;
	if (bIsReady == bInReady) return;

	bIsReady = bInReady;
	OnReadyChanged.Broadcast(bIsReady);
}

void ATurnBasedPlayerState::ResetTurnState()
{
	if (!HasAuthority()) return;

	TurnsMissed = 0;
	bForfeited  = false;
	bIsReady    = false;
}

// --- RepNotify ---

void ATurnBasedPlayerState::OnRep_TurnsMissed()
{
	OnTurnsMissedChanged.Broadcast(TurnsMissed);
}

void ATurnBasedPlayerState::OnRep_Forfeited()
{
	if (bForfeited) OnForfeited.Broadcast();
}

void ATurnBasedPlayerState::OnRep_Ready()
{
	OnReadyChanged.Broadcast(bIsReady);
}