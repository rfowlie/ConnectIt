// Fill out your copyright notice in the Description page of Project Settings.


#include "GameIntelligence/InfluenceMap/PieceControlInfluenceMap.h"
#include "GridMechanics_GridLibrary.h"
#include "Board/ConnectIt_BoardStateComponent.h"


void UPieceControlInfluenceMap::SetUpdate(UConnectIt_BoardStateComponent* InBoardStateComponent)
{
	if (!InBoardStateComponent) return;
	BoardStateComponent = InBoardStateComponent;
	InBoardStateComponent->OnBoardStateChanged.AddUniqueDynamic(this, &ThisClass::Update);
}

float UPieceControlInfluenceMap::GetData(const FIntPoint InPoint) const
{
	if (InfluenceMap.Contains(InPoint)) return InfluenceMap[InPoint];
	return 0;
}

void UPieceControlInfluenceMap::Update()
{
	if (!BoardStateComponent) return;
	auto Snapshot = BoardStateComponent->GetBoardSnapshot()->CurrentState;

	// TODO: check if faction turn set properly on board snapshot, otherwise -1 will cause problems

	InfluenceMap.Empty();
	for (const auto GridPosition : Snapshot.TilePositions)
	{
		int32 Count = 0;
		// get all surrounding possible tiles mathematically
		for (auto Neighbor : UGridMechanics_GridLibrary::GetPositionNeighbors(GridPosition))
		{
			if (const auto TileData = Snapshot.GetTileData(Neighbor))
			{
				Count += TileData->FactionPiece == Snapshot.FactionTurn ? 1 : -1;
			}
		}

		InfluenceMap.Add(FIntPoint(GridPosition.X, GridPosition.Y), Count);
	}
}
