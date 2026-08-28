// Fill out your copyright notice in the Description page of Project Settings.


#include "GridMechanics_GridLibrary.h"
#include "GridMechanicsBaseEnums.h"
#include "GridMechanicsBaseStructs.h"
#include "Tile/GridTileBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridMechanics, Log, All);

const TMap<EGridDirection, FGridDirectionVector> UGridMechanics_GridLibrary::GridDirectionVectors =
{
	{ EGridDirection::Up,        {  0,  1 } },
	{ EGridDirection::UpRight,   {  1,  1 } },
	{ EGridDirection::Right,     {  1,  0 } },
	{ EGridDirection::DownRight, {  1, -1 } },
	{ EGridDirection::Down,      {  0, -1 } },
	{ EGridDirection::DownLeft,  { -1, -1 } },
	{ EGridDirection::Left,      { -1,  0 } },
	{ EGridDirection::UpLeft,    { -1,  1 } }
};

// ============================================================
// Direction <-> Vector / Rotation
// ============================================================

bool UGridMechanics_GridLibrary::GridPositionIsEqual(const FGridPosition& A, const FGridPosition& B)
{
	return A == B;
}

FGridDirectionVector UGridMechanics_GridLibrary::GetGridDirectionVector(EGridDirection Direction)
{
	// FindRef returns a default-constructed FGridDirectionVector (0,0) if Direction isn't a valid key
	// (e.g. EGridDirection::Max). This is intentional so callers get a harmless no-op vector rather
	// than a crash, but it means lookup failures are silent - callers relying on a "real" direction
	// should validate Direction != EGridDirection::Max first.
	return GridDirectionVectors.FindRef(Direction);
}

EGridDirection UGridMechanics_GridLibrary::GetGridDirectionFromDegrees(float InDegrees)
{
	// Normalize into [0, 360) then bucket into 8 45-degree segments.
	const float NormalizedDegrees = static_cast<float>((FMath::RoundToInt(InDegrees) % 360 + 360) % 360);
	const int32 DirectionIndex = FMath::RoundToInt(NormalizedDegrees / 45.f) % 8;
	return static_cast<EGridDirection>(DirectionIndex);
}

float UGridMechanics_GridLibrary::GetRotationFromGridDirection(EGridDirection InDirection)
{
	return static_cast<float>(static_cast<int32>(InDirection) * 45);
}

EGridDirection UGridMechanics_GridLibrary::GetActorGridDirection(const AActor* InActor)
{
	if (!IsValid(InActor)) { return EGridDirection::Max; }
	return GetGridDirectionFromDegrees(InActor->GetActorRotation().Yaw);
}

EGridDirection UGridMechanics_GridLibrary::GetRotatedGridDirection(EGridDirection Direction, EGridDirection Forward)
{
	// Wrap around the 8-way compass rather than letting the sum overflow past EGridDirection::UpLeft.
	constexpr int32 NumDirections = 8;
	const int32 Rotated = (static_cast<int32>(Direction) + static_cast<int32>(Forward)) % NumDirections;
	return static_cast<EGridDirection>(Rotated);
}

void UGridMechanics_GridLibrary::GetNormalizedGridPositionByDirection(FGridPosition& OutGridPosition, EGridDirection GridDirection)
{
	if (const FGridDirectionVector* DirectionVector = GridDirectionVectors.Find(GridDirection))
	{
		OutGridPosition = FGridPosition(DirectionVector->Row, DirectionVector->Column);
	}
}

// ============================================================
// Grid Position Math
// ============================================================

FGridPosition UGridMechanics_GridLibrary::CalculateGridPositionFromSize(const AActor* Actor, int32 Size)
{
	if (Size < 10)
	{
		UE_LOG(LogGridMechanics, Warning, TEXT("CalculateGridPositionFromSize: Size %d clamped to minimum of 10"), Size);
	}
	Size = FMath::Max(10, Size);

	FGridPosition GridPosition;
	if (IsValid(Actor))
	{
		GridPosition.X = FMath::TruncToInt(Actor->GetActorLocation().X / Size);
		GridPosition.Y = FMath::TruncToInt(Actor->GetActorLocation().Y / Size);
	}

	return GridPosition;
}

FGridPosition UGridMechanics_GridLibrary::GetAverageGridPosition(const TArray<FGridPosition>& InGridPositions)
{
	if (InGridPositions.IsEmpty())
	{
		UE_LOG(LogGridMechanics, Warning, TEXT("GetAverageGridPosition called with an empty array; returning (0,0)"));
		return FGridPosition(0, 0);
	}

	FGridPosition OutGridPosition;
	for (const FGridPosition& GridPosition : InGridPositions)
	{
		OutGridPosition += GridPosition;
	}
	return OutGridPosition / InGridPositions.Num();
}

void UGridMechanics_GridLibrary::GetClosestNormalizedGridPositionFromPositions(FGridPosition& OutGridPosition,
	const FGridPosition& P1, const FGridPosition& P2)
{
	const FGridPosition Delta = P2 - P1;
	if (Delta.X == 0 && Delta.Y == 0)
	{
		OutGridPosition = FGridPosition(0, 0);
		return;
	}

	const int32 AbsX = FMath::Abs(Delta.X);
	const int32 AbsY = FMath::Abs(Delta.Y);

	const int32 SignX = FMath::Sign(Delta.X);
	const int32 SignY = FMath::Sign(Delta.Y);

	if (AbsX == AbsY)
	{
		OutGridPosition = FGridPosition(SignX, SignY);
	}
	else if (AbsX > AbsY)
	{
		OutGridPosition = FGridPosition(SignX, 0);
	}
	else
	{
		OutGridPosition = FGridPosition(0, SignY);
	}
}

EGridDirection UGridMechanics_GridLibrary::GetClosestGridDirectionBetweenPositions(const FGridPosition& P1, const FGridPosition& P2)
{
	if (P1 == P2)
	{
		UE_LOG(LogGridMechanics, Warning, TEXT("GetClosestGridDirectionBetweenPositions: P1 == P2, no direction is well-defined; defaulting to Up"));
		return EGridDirection::Up;
	}

	FGridPosition NormalizedOffset;
	GetClosestNormalizedGridPositionFromPositions(NormalizedOffset, P1, P2);

	// Reverse-lookup the normalized offset against the canonical direction vectors rather than relying
	// on an external GridPositionGridDirectionMap (not defined anywhere in this library), so this
	// function is self-contained and can't drift out of sync with GridDirectionVectors.
	for (const auto& [GridDirection, DirectionVector] : UGridMechanics_GridLibrary::GridDirectionVectors)
	{
		if (DirectionVector.Row == NormalizedOffset.X && DirectionVector.Column == NormalizedOffset.Y)
		{
			return GridDirection;
		}
	}

	UE_LOG(LogGridMechanics, Warning, TEXT("GetClosestGridDirectionBetweenPositions: no matching direction found for offset (%d, %d); defaulting to Up"), NormalizedOffset.X, NormalizedOffset.Y);
	return EGridDirection::Up;
}

// ============================================================
// Neighbors & Traversal
// ============================================================

TArray<FGridPosition> UGridMechanics_GridLibrary::GetPositionNeighbors(const FGridPosition InPosition)
{
	TArray<FGridPosition> Neighbors;
	Neighbors.Reserve(UGridMechanics_GridLibrary::GridDirectionVectors.Num());

	for (const auto& [GridDirection, DirectionVector] : UGridMechanics_GridLibrary::GridDirectionVectors)
	{
		Neighbors.Add(FGridPosition(InPosition.X + DirectionVector.Row, InPosition.Y + DirectionVector.Column));
	}

	return Neighbors;
}

// ============================================================
// Tile Utility
// ============================================================

AGridTileBase* UGridMechanics_GridLibrary::GetRandomGridTile(const TArray<AGridTileBase*> InTiles)
{
	if (InTiles.IsEmpty()) { return nullptr; }
	return InTiles[FMath::RandRange(0, InTiles.Num() - 1)];
}

// ============================================================
// Connection Analysis (N-in-a-row)
// ============================================================

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

		if (bWindowValid) { ValidWindows++; }
	}

	return ValidWindows;
}

int32 UGridMechanics_GridLibrary::CountPossibleGridConnections(
	const FGridPosition& StartPosition, const TSet<FGridPosition>& ValidPositions, int32 ConnectLength)
{
	if (!ValidPositions.Contains(StartPosition)) { return 0; }

	int32 TotalWindows = 0;
	for (const auto& [GridDirection, DirectionVector] : UGridMechanics_GridLibrary::GridDirectionVectors)
	{
		TotalWindows += CountValidWindows(StartPosition, ValidPositions, DirectionVector, ConnectLength);
	}

	return TotalWindows;
}

TMap<EGridDirection, int32> UGridMechanics_GridLibrary::GetPotentialConnectionCountPerDirection(
	const FGridPosition& Position, const TSet<FGridPosition>& ValidPositions, int32 ConnectLength)
{
	TMap<EGridDirection, int32> OutCounts;

	if (!ValidPositions.Contains(Position)) { return OutCounts; }

	for (const auto& [GridDirection, DirectionVector] : UGridMechanics_GridLibrary::GridDirectionVectors)
	{
		OutCounts.Add(GridDirection, CountValidWindows(Position, ValidPositions, DirectionVector, ConnectLength));
	}

	return OutCounts;
}

TArray<TPair<int32, FGridPosition>> UGridMechanics_GridLibrary::CreateConnectionsFloodMap(
	const TArray<FGridPosition>& GridPositions, const int32 ConnectionLength)
{
	TArray<TPair<int32, FGridPosition>> ConnectionsFloodMap;
	ConnectionsFloodMap.Reserve(GridPositions.Num());

	const TSet<FGridPosition> ValidPositions = TSet(GridPositions);
	for (const FGridPosition& GridPosition : GridPositions)
	{
		const int32 Connections = UGridMechanics_GridLibrary::CountPossibleGridConnections(GridPosition, ValidPositions, ConnectionLength);
		ConnectionsFloodMap.Add(TPair<int32, FGridPosition>(Connections, GridPosition));
	}

	Algo::Sort(ConnectionsFloodMap, [](const TPair<int32, FGridPosition>& A, const TPair<int32, FGridPosition>& B)
	{
		return A.Key > B.Key;
	});

	return ConnectionsFloodMap;
}