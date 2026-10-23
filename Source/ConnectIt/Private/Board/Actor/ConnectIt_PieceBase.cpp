// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/Actor/ConnectIt_PieceBase.h"


AConnectIt_PieceBase::AConnectIt_PieceBase()
{
	// Local to each machine (see the class comment)
	bReplicates = false;
	PrimaryActorTick.bCanEverTick = false;
}

void AConnectIt_PieceBase::InitialisePiece(
	int32 InFactionID,
	FGridPosition InGridPosition)
{
	FactionID        = InFactionID;
	OccupiedPosition = InGridPosition;
	GridPosition     = InGridPosition;

	OnFactionVisualUpdate(FactionID);
}

void AConnectIt_PieceBase::OnFactionVisualUpdate_Implementation(int32 InFactionID)
{
	// Base does nothing
	// Blueprint subclass sets mesh, material, VFX based on FactionID
}
