// Fill out your copyright notice in the Description page of Project Settings.


#include "Tile/GridTileRegistryComponent.h"
#include "Tile/GridTileBase.h"
#include "Subsystem/GridWorldSubsystem.h"


UGridTileRegistryComponent::UGridTileRegistryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGridTileRegistryComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!ResolveSubsystem())
    {
        UE_LOG(LogTemp, Error,
            TEXT("GridRegistryComponent: Failed to resolve UGridWorldSubsystem"));
        return;
    }

#if WITH_EDITOR
    ValidateTileAlignment();
#endif
}

bool UGridTileRegistryComponent::ResolveSubsystem()
{
    GridSubsystem = GetWorld()->GetSubsystem<UGridWorldSubsystem>();
    return IsValid(GridSubsystem);
}

// --- Position Conversion ---

FVector UGridTileRegistryComponent::GridPositionToWorld(const FGridPosition Position) const
{
    return FVector(
        Position.X * GridSize,
        Position.Y * GridSize,
        GetOwner()->GetActorLocation().Z
    );
}

FGridPosition UGridTileRegistryComponent::WorldToGridPosition(const FVector& WorldLocation) const
{
    return FGridPosition(
        FMath::RoundToInt32(WorldLocation.X / GridSize),
        FMath::RoundToInt32(WorldLocation.Y / GridSize)
    );
}

// --- Tile Queries ---

AGridTileBase* UGridTileRegistryComponent::GetTile(const FGridPosition Position) const
{
    if (!IsValid(GridSubsystem)) return nullptr;
    return GridSubsystem->GetTileAtPosition(Position);
}

TArray<AGridTileBase*> UGridTileRegistryComponent::GetRow(const int32 RowIndex) const
{
    TArray<AGridTileBase*> OutTiles;
    if (!IsValid(GridSubsystem)) return OutTiles;

    for (const FGridPosition& Position : GetRowPositions(RowIndex))
    {
        if (AGridTileBase* Tile = GridSubsystem->GetTileAtPosition(Position))
        {
            OutTiles.Add(Tile);
        }
    }

    return OutTiles;
}

TArray<AGridTileBase*> UGridTileRegistryComponent::GetColumn(const int32 ColumnIndex) const
{
    TArray<AGridTileBase*> OutTiles;
    if (!IsValid(GridSubsystem)) return OutTiles;

    for (const FGridPosition& Position : GetColumnPositions(ColumnIndex))
    {
        if (AGridTileBase* Tile = GridSubsystem->GetTileAtPosition(Position))
        {
            OutTiles.Add(Tile);
        }
    }

    return OutTiles;
}

TArray<FGridPosition> UGridTileRegistryComponent::GetAllTilePositions() const
{
    if (!IsValid(GridSubsystem)) return TArray<FGridPosition>();
    return GridSubsystem->GetAllTilePositions();
}

TArray<AGridTileBase*> UGridTileRegistryComponent::GetAllTiles() const
{
    if (!IsValid(GridSubsystem)) return TArray<AGridTileBase*>();
    return GridSubsystem->GetAllTiles();
}

// --- Board Dimension Queries ---

int32 UGridTileRegistryComponent::GetMinRow() const
{
    int32 Min = TNumericLimits<int32>::Max();
    for (const FGridPosition& Pos : GetAllTilePositions())
    {
        Min = FMath::Min(Min, Pos.X);
    }
    return Min == TNumericLimits<int32>::Max() ? 0 : Min;
}

int32 UGridTileRegistryComponent::GetMaxRow() const
{
    int32 Max = TNumericLimits<int32>::Min();
    for (const FGridPosition& Pos : GetAllTilePositions())
    {
        Max = FMath::Max(Max, Pos.X);
    }
    return Max == TNumericLimits<int32>::Min() ? 0 : Max;
}

int32 UGridTileRegistryComponent::GetMinColumn() const
{
    int32 Min = TNumericLimits<int32>::Max();
    for (const FGridPosition& Pos : GetAllTilePositions())
    {
        Min = FMath::Min(Min, Pos.Y);
    }
    return Min == TNumericLimits<int32>::Max() ? 0 : Min;
}

int32 UGridTileRegistryComponent::GetMaxColumn() const
{
    int32 Max = TNumericLimits<int32>::Min();
    for (const FGridPosition& Pos : GetAllTilePositions())
    {
        Max = FMath::Max(Max, Pos.Y);
    }
    return Max == TNumericLimits<int32>::Min() ? 0 : Max;
}

int32 UGridTileRegistryComponent::GetRowCount() const
{
    const TArray<FGridPosition> Positions = GetAllTilePositions();
    if (Positions.IsEmpty()) return 0;

    TSet<int32> UniqueRows;
    for (const FGridPosition& Pos : Positions)
    {
        UniqueRows.Add(Pos.X);
    }
    return UniqueRows.Num();
}

int32 UGridTileRegistryComponent::GetColumnCount() const
{
    const TArray<FGridPosition> Positions = GetAllTilePositions();
    if (Positions.IsEmpty()) return 0;

    TSet<int32> UniqueColumns;
    for (const FGridPosition& Pos : Positions)
    {
        UniqueColumns.Add(Pos.Y);
    }
    return UniqueColumns.Num();
}

TArray<FGridPosition> UGridTileRegistryComponent::GetRowPositions(const int32 RowIndex) const
{
    TArray<FGridPosition> OutPositions;
    for (const FGridPosition& Pos : GetAllTilePositions())
    {
        if (Pos.X == RowIndex)
        {
            OutPositions.Add(Pos);
        }
    }
    return OutPositions;
}

TArray<FGridPosition> UGridTileRegistryComponent::GetColumnPositions(const int32 ColumnIndex) const
{
    TArray<FGridPosition> OutPositions;
    for (const FGridPosition& Pos : GetAllTilePositions())
    {
        if (Pos.Y == ColumnIndex)
        {
            OutPositions.Add(Pos);
        }
    }
    return OutPositions;
}

#if WITH_EDITOR
void UGridTileRegistryComponent::ValidateTileAlignment() const
{
    if (!IsValid(GridSubsystem)) return;

    TArray<AGridTileBase*> AllTiles = GridSubsystem->GetAllTiles();
    int32 MisalignedCount = 0;

    for (AGridTileBase* Tile : AllTiles)
    {
        if (!IsValid(Tile)) continue;

        const FVector WorldLocation  = Tile->GetActorLocation();
        const FGridPosition Expected = WorldToGridPosition(WorldLocation);
        // const FGridPosition Actual   = Tile->GetGridPosition();
        const FGridPosition Actual   = FGridPosition();

        if (Expected != Actual)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("GridRegistryComponent: Tile %s is misaligned — "
                     "GridPosition (%d,%d) does not match world position (%d,%d). "
                     "Expected GridPosition based on world location with GridSize %d."),
                *Tile->GetName(),
                Actual.X, Actual.Y,
                Expected.X, Expected.Y,
                GridSize);

            MisalignedCount++;
        }
    }

    if (MisalignedCount == 0)
    {
        UE_LOG(LogTemp, Log,
            TEXT("GridRegistryComponent: All %d tiles validated successfully"),
            AllTiles.Num());
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GridRegistryComponent: %d misaligned tile(s) found — "
                 "check GridSize (%d) matches tile placement"),
            MisalignedCount, GridSize);
    }
}
#endif
