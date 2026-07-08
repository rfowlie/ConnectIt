// Fill out your copyright notice in the Description page of Project Settings.


#include "Grid/ConnectIt_GridPiece.h"
#include "Net/UnrealNetwork.h"


AConnectIt_GridPiece::AConnectIt_GridPiece()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
}

void AConnectIt_GridPiece::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AConnectIt_GridPiece, FactionID);
	DOREPLIFETIME(AConnectIt_GridPiece, OccupiedPosition);
}

void AConnectIt_GridPiece::InitialisePiece(
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

void AConnectIt_GridPiece::OnRep_FactionID()
{
	OnFactionVisualUpdate(FactionID);
}

void AConnectIt_GridPiece::OnFactionVisualUpdate_Implementation(int32 InFactionID)
{
	// Base does nothing
	// Blueprint subclass sets mesh, material, VFX based on FactionID
}
