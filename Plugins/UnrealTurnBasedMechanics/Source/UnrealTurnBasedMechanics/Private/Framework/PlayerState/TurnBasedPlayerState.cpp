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

	DOREPLIFETIME(ATurnBasedPlayerState, TurnsMissed);
	DOREPLIFETIME(ATurnBasedPlayerState, bForfeited);
	DOREPLIFETIME(ATurnBasedPlayerState, bIsReady);
	DOREPLIFETIME(ATurnBasedPlayerState, ParticipantType);
}

void ATurnBasedPlayerState::Server_IncrementTurnsMissed()
{
	check(!IsRunningClientOnly());
	TurnsMissed++;
}

void ATurnBasedPlayerState::Server_SetForfeited()
{
	check(!IsRunningClientOnly());
	bForfeited = true;
	OnForfeited.Broadcast();
}

void ATurnBasedPlayerState::Server_SetReady(bool bReady)
{
	check(!IsRunningClientOnly());
	bIsReady = bReady;
}

void ATurnBasedPlayerState::OnRep_TurnsMissed()
{
	// Clients can bind UI here if needed
}

void ATurnBasedPlayerState::OnRep_Forfeited()
{
	OnForfeited.Broadcast();
}

void ATurnBasedPlayerState::OnRep_Ready()
{
	OnReadyChanged.Broadcast();
}