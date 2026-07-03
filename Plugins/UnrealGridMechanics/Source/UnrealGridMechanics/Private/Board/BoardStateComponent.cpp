// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/BoardStateComponent.h"
#include "UnrealGridMechanics/Public/Shift/GridShiftTypes.h"


UBoardStateComponent::UBoardStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UBoardStateComponent::HasTileData(const FGridPosition Position) const
{
	return TileDataMap.Contains(Position);
}

FTileData UBoardStateComponent::GetTileData(const FGridPosition Position) const
{
	const FTileData* Found = TileDataMap.Find(Position);
	return Found ? *Found : FTileData();
}

TArray<FGridPosition> UBoardStateComponent::GetAllPositions() const
{
	TArray<FGridPosition> Out;
	TileDataMap.GenerateKeyArray(Out);
	return Out;
}

void UBoardStateComponent::SetTileData(const FGridPosition Position, const FTileData Data)
{
	TileDataMap.Add(Position, Data);
	OnTileDataChanged.Broadcast(Position, Data);
}

void UBoardStateComponent::RemoveTileData(const FGridPosition Position)
{
	TileDataMap.Remove(Position);
	OnBoardStateUpdated.Broadcast();
}

void UBoardStateComponent::ClearAllTileData()
{
	TileDataMap.Empty();
	OnBoardStateUpdated.Broadcast();
}

void UBoardStateComponent::ApplyShiftResult(const FShiftResult& Result)
{
	// Snapshot affected tiles before mutating
	// Prevents overwriting data mid-remap
	TMap<FGridPosition, FTileData> Snapshot;

	for (const auto& [OldPos, NewPos] : Result.PositionRemap)
	{
		if (const FTileData* Data = TileDataMap.Find(OldPos))
		{
			Snapshot.Add(OldPos, *Data);
		}
	}

	// Apply remap from snapshot — atomic, no mid-operation overwrites
	for (const auto& [OldPos, NewPos] : Result.PositionRemap)
	{
		if (const FTileData* Data = Snapshot.Find(OldPos))
		{
			TileDataMap.Add(NewPos, *Data);
		}
		else
		{
			// No data at old position — clear the destination
			TileDataMap.Remove(NewPos);
		}
	}

	OnBoardStateUpdated.Broadcast();
}
