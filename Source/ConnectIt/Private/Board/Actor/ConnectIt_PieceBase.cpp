// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/Actor/ConnectIt_PieceBase.h"
#include "Board/ConnectIt_BoardVisualizer.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "Piece/GridPieceRegistryBase.h"


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

void AConnectIt_PieceBase::RefreshFromBoard_Implementation()
{
	const UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
	const UGridPieceRegistryBase* Pieces = UConnectIt_GameUtilityLibrary::GetPieceRegistry(this);
	FGridPosition Position;
	if (!BoardState || !Pieces || !Pieces->FindPositionOfPiece(this, Position)) return;

	// The registry's position is the true one (moves, shifts and swaps keep it)
	OccupiedPosition = Position;
	GridPosition     = Position;

	const FConnectItTileData* Data = BoardState->GetDisplayBoard().GetTileData(Position);
	if (Data && Data->bIsOccupied && Data->FactionPiece != FactionID)
	{
		FactionID = Data->FactionPiece;
		OnFactionVisualUpdate(FactionID);
	}
}

void AConnectIt_PieceBase::SetBoardPosition(const FGridPosition& Position)
{
	OccupiedPosition = Position;
	GridPosition     = Position;
}

void AConnectIt_PieceBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AConnectIt_BoardVisualizer* Visualizer = AConnectIt_BoardVisualizer::GetBoardVisualizer(this))
	{
		Visualizer->ReleaseHoldsBy(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AConnectIt_PieceBase::OnFactionVisualUpdate_Implementation(int32 InFactionID)
{
	// Base does nothing
	// Blueprint subclass sets mesh, material, VFX based on FactionID
}
