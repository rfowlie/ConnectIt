// Fill out your copyright notice in the Description page of Project Settings.


#include "GridMechanics_GridLibrary.h"
#include "GridMechanicsBaseEnums.h"
#include "GridMechanicsBaseStructs.h"


const TMap<EGridDirection, FGridDirectionVector> UGridMechanics_GridLibrary::GridDirectionVectors =
{
	{ EGridDirection::Up,       { 0,  1 } },
	{ EGridDirection::UpRight,  { 1,  1 } },
	{ EGridDirection::Right,    { 1,  0 } },
	{ EGridDirection::DownRight,{ 1, -1 } },
	{ EGridDirection::Down,     { 0, -1 } },
	{ EGridDirection::DownLeft, { -1, -1 } },
	{ EGridDirection::Left,     { -1, 0 } },
	{ EGridDirection::UpLeft,   { -1, 1 } }
};

FGridDirectionVector UGridMechanics_GridLibrary::GetGridDirectionVector(EGridDirection Direction)
{
	return GridDirectionVectors.FindRef(Direction);
}

FGridPosition UGridMechanics_GridLibrary::CalculateGridPositionFromSize(const AActor* Actor, int32 Size)
{
	Size = FMath::Max(10, Size);
	FGridPosition GridPosition;
	if (IsValid(Actor))
	{
		GridPosition.X = FMath::TruncToInt(Actor->GetActorLocation().X / Size);
		GridPosition.Y = FMath::TruncToInt(Actor->GetActorLocation().Y / Size);
	}

	// UE_LOG(LogTemp, Display, TEXT("Grid Position: %d %d"), GridPosition.X, GridPosition.Y);
	
	return GridPosition;
}

int32 UGridMechanics_GridLibrary::CountValidWindows(
	const FGridPosition& Position, const TSet<FGridPosition>& ValidPositions, const FGridDirectionVector& Direction, int32 ConnectLength)
{
	int32 ValidWindows = 0;

	// Slide window start from -(ConnectLength-1) to 0
	// Each start offset represents the window beginning at that offset from Position
	for (int32 Offset = -(ConnectLength - 1); Offset <= 0; Offset++)
	{
		bool bWindowValid = true;

		// Check all ConnectLength positions in this window
		for (int32 Step = 0; Step < ConnectLength; Step++)
		{
			const int32 Index = Offset + Step;
			const FGridPosition Check(
				Position.X + Index * Direction.Row,
				Position.Y + Index * Direction.Column
			);

			if (!ValidPositions.Contains(Check))
			{
				bWindowValid = false;
				break;
			}
		}

		if (bWindowValid) ValidWindows++;
	}

	return ValidWindows;
}

int32 UGridMechanics_GridLibrary::CountPossibleGridConnections(
	const FGridPosition& StartPosition, const TSet<FGridPosition>& ValidPositions, int32 ConnectLength)
{
	if (!ValidPositions.Contains(StartPosition)) return 0;

	int32 TotalWindows = 0;

	for (const auto& [GridDirection, DirectionVector] : GridDirectionVectors)
	{
		TotalWindows += CountValidWindows(
			StartPosition,
			ValidPositions,
			DirectionVector,
			ConnectLength
		);
	}

	return TotalWindows;
}

TArray<TPair<int32, FGridPosition>> UGridMechanics_GridLibrary::CreateConnectionsFloodMap(
	const TArray<FGridPosition>& GridPositions, const int32 ConnectionLength)
{
	TArray<TPair<int32, FGridPosition>> ConnectionsFloodMap;
	const TSet<FGridPosition> ValidPositions = TSet(GridPositions);
	for (const auto& GridPosition : GridPositions)
	{
		int32 Connections = UGridMechanics_GridLibrary::CountPossibleGridConnections(GridPosition, ValidPositions, ConnectionLength);
		ConnectionsFloodMap.Add(TPair<int32, FGridPosition>(Connections, GridPosition));
	}

	Algo::Sort(ConnectionsFloodMap, [](const TPair<int32, FGridPosition>& A, const TPair<int32, FGridPosition>& B)
	{
		return A.Key > B.Key;
	});


	return ConnectionsFloodMap;
}

// TMap<EGridDirection, int32> UGridMechanics_GridLibrary::GetPotentialConnectFourCountPerDirection(
// 	const FGridPosition& Position, const TSet<FGridPosition>& ValidPositions, const int32 ConnectLength)
// {
// 	TMap<EGridDirection, int32> OutCounts;
//
// 	if (!ValidPositions.Contains(Position)) return OutCounts;
//
// 	for (const auto& [GridDirection, DirectionVector] : GridDirectionVectors)
// 	{
// 		OutCounts.Add(GridDirection, CountValidWindows(
// 			Position,
// 			ValidPositions,
// 			DirectionVector,
// 			ConnectLength
// 		));
// 	}
//
// 	return OutCounts;
// }
