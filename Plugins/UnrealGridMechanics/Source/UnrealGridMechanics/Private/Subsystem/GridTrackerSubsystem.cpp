// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/GridTrackerSubsystem.h"
#include "Tile/GridTileBase.h"
#include "Unit/GridUnitBase.h"


void UGridTrackerSubsystem::PostInitialize()
{
	Super::PostInitialize();
}

void UGridTrackerSubsystem::OnBeginCursorOverGridTile(AGridTileBase* GridTileBase)
{
	if (!IsValid(GridTileBase)) { return; }
	
	// update tile
	if (IsValid(GridTileHovered))
	{
		if (OnGridTileHoveredStop.IsBound())
		{
			OnGridTileHoveredStop.Broadcast(GridTileHovered);
		}
		
		GridTileHovered = nullptr;
	}
		
	GridTileHovered = GridTileBase;
	if (OnGridTileHoveredStart.IsBound())
	{
		OnGridTileHoveredStart.Broadcast(GridTileHovered);
	}
}

void UGridTrackerSubsystem::RegisterGridTile(AGridTileBase* GridTile)
{
	if (IsValid(GridTile) && !GridTilesAll.Contains(GridTile))
	{
		GridTilesAll.AddUnique(GridTile);

		GridTile->OnGridTileBeginCursorOver.AddDynamic(this, &ThisClass::OnBeginCursorOverGridTile);
	}
}

void UGridTrackerSubsystem::UnregisterGridTile(AGridTileBase* GridTile)
{
	if (IsValid(GridTile) && !GridTilesAll.Contains(GridTile))
	{
		GridTilesAll.Remove(GridTile);
	}
}

void UGridTrackerSubsystem::RegisterGridUnit(AGridUnitBase* GridUnit)
{
	if (IsValid(GridUnit) && !GridUnitsAll.Contains(GridUnit))
	{
		GridUnitsAll.AddUnique(GridUnit);
	}
}

void UGridTrackerSubsystem::UnregisterGridUnit(AGridUnitBase* GridUnit)
{
	if (IsValid(GridUnit) && GridUnitsAll.Contains(GridUnit))
	{
		GridUnitsAll.Remove(GridUnit);
	}
}
