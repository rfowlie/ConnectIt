// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/Actor/ConnectIt_PieceBase.h"
#include "Net/UnrealNetwork.h"


AConnectIt_PieceBase::AConnectIt_PieceBase()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
}

void AConnectIt_PieceBase::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AConnectIt_PieceBase, FactionID);
	DOREPLIFETIME(AConnectIt_PieceBase, OccupiedPosition);
}

void AConnectIt_PieceBase::InitialisePiece(
	int32 InFactionID,
	FGridPosition InGridPosition)
{
	FactionID        = InFactionID;
	OccupiedPosition = InGridPosition;
	GridPosition     = InGridPosition;

	// Drive visuals immediately on server
	// Clients will receive via OnRep_FactionID
	OnFactionVisualUpdate(FactionID);
}

void AConnectIt_PieceBase::OnRep_FactionID()
{
	OnFactionVisualUpdate(FactionID);
}

void AConnectIt_PieceBase::OnFactionVisualUpdate_Implementation(int32 InFactionID)
{
	// Base does nothing
	// Blueprint subclass sets mesh, material, VFX based on FactionID
}
