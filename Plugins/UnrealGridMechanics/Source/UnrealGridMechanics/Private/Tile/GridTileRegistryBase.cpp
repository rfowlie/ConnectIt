// Fill out your copyright notice in the Description page of Project Settings.


#include "Tile/GridTileRegistryBase.h"

#include "Tile/GridTileBase.h"
#include "Subsystem/GridHoverSubsystem.h"
#include "Algo/Sort.h"
#include "EngineUtils.h"


UGridHoverSubsystem* UGridTileRegistryBase::ResolveHoverSubsystem()
{
    if (!IsValid(HoverSubsystem))
    {
        HoverSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UGridHoverSubsystem>() : nullptr;
    }
    return HoverSubsystem;
}

// --- Lifecycle ---

void UGridTileRegistryBase::InitialiseRegistry()
{
    DiscoverTiles();

    if (UGridHoverSubsystem* Hover = ResolveHoverSubsystem())
    {
        for (const TObjectPtr<AGridTileBase>& Tile : Tiles)
        {
            Hover->RegisterTile(Tile);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GridTileRegistryBase: no UGridHoverSubsystem - hover relay disabled, queries still work"));
    }

    if (UWorld* World = GetWorld())
    {
        ActorSpawnedHandle = World->AddOnActorSpawnedHandler(
            FOnActorSpawned::FDelegate::CreateUObject(this, &UGridTileRegistryBase::HandleActorSpawned));
    }
}

void UGridTileRegistryBase::ShutdownRegistry()
{
    if (UWorld* World = GetWorld(); World && ActorSpawnedHandle.IsValid())
    {
        World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
        ActorSpawnedHandle.Reset();
    }

    if (IsValid(HoverSubsystem))
    {
        for (const TObjectPtr<AGridTileBase>& Tile : Tiles)
        {
            if (IsValid(Tile))
            {
                HoverSubsystem->UnregisterTile(Tile);
            }
        }
    }
    Tiles.Reset();
}

void UGridTileRegistryBase::DiscoverTiles()
{
    Tiles.Reset();

    const UWorld* World = GetWorld();
    if (!World) return;

    for (TActorIterator<AGridTileBase> It(World); It; ++It)
    {
        if (IsValid(*It))
        {
            Tiles.AddUnique(*It);
        }
    }

    // TODO: assign current tiles to their location which is calculated using this registry?
    Algo::Sort(Tiles, [](const TObjectPtr<AGridTileBase>& A, const TObjectPtr<AGridTileBase>& B)
    {
        return GetNameSafe(A.Get()) < GetNameSafe(B.Get());
    });
}

void UGridTileRegistryBase::HandleActorSpawned(AActor* SpawnedActor)
{
    AGridTileBase* Tile = Cast<AGridTileBase>(SpawnedActor);
    if (!IsValid(Tile)) return;
    if (Tiles.Contains(Tile)) return;

    Tiles.Add(Tile);
    if (IsValid(HoverSubsystem))
    {
        HoverSubsystem->RegisterTile(Tile);
    }
}

// --- Position Conversion ---

FVector UGridTileRegistryBase::GridPositionToWorld_Implementation(FGridPosition Position) const
{
    // The ActorComponent version anchors Z to GetOwner()->GetActorLocation().Z --
    // a plain UObject has no equivalent without an explicit owner reference
    // threaded through, which this prototype deliberately doesn't add (see class
    // comment). Flat Z=0 here; a concrete subclass with real owner/board-plane
    // knowledge can override this whole function to restore that behavior.
    return FVector(
        Position.X * GridSize,
        Position.Y * GridSize,
        0.f);
}

FGridPosition UGridTileRegistryBase::WorldToGridPosition_Implementation(const FVector& WorldLocation) const
{
    return FGridPosition(
        FMath::RoundToInt32(WorldLocation.X / GridSize),
        FMath::RoundToInt32(WorldLocation.Y / GridSize));
}

// --- Tile Queries ---

AGridTileBase* UGridTileRegistryBase::GetTileAtPosition(FGridPosition Position) const
{
    for (const TObjectPtr<AGridTileBase>& Tile : Tiles)
    {
        if (IsValid(Tile) && Position == WorldToGridPosition(Tile->GetActorLocation()))
        {
            return Tile;
        }
    }
    
    return nullptr;
}

FGridPosition UGridTileRegistryBase::GetPositionOfTile(const AGridTileBase* Tile) const
{
    if (!IsValid(Tile)) return FGridPosition();
    return WorldToGridPosition(Tile->GetActorLocation());
}

TArray<AGridTileBase*> UGridTileRegistryBase::GetRow(int32 RowIndex) const
{
    TArray<AGridTileBase*> OutTiles;
    for (const FGridPosition& Position : GetRowPositions(RowIndex))
    {
        if (AGridTileBase* Tile = GetTileAtPosition(Position))
        {
            OutTiles.Add(Tile);
        }
    }
    return OutTiles;
}

TArray<AGridTileBase*> UGridTileRegistryBase::GetColumn(int32 ColumnIndex) const
{
    TArray<AGridTileBase*> OutTiles;
    for (const FGridPosition& Position : GetColumnPositions(ColumnIndex))
    {
        if (AGridTileBase* Tile = GetTileAtPosition(Position))
        {
            OutTiles.Add(Tile);
        }
    }
    return OutTiles;
}

TArray<FGridPosition> UGridTileRegistryBase::GetAllTilePositions() const
{
    TArray<FGridPosition> OutPositions;
    for (const TObjectPtr<AGridTileBase>& Tile : Tiles)
    {
        if (IsValid(Tile))
        {
            OutPositions.Add(GetPositionOfTile(Tile));
        }
    }
    return OutPositions;
}

TArray<AGridTileBase*> UGridTileRegistryBase::GetAllTiles() const
{
    TArray<AGridTileBase*> Out;
    Out.Reserve(Tiles.Num());
    for (const TObjectPtr<AGridTileBase>& Tile : Tiles)
    {
        if (IsValid(Tile))
        {
            Out.Add(Tile);
        }
    }
    return Out;
}

// --- Board Dimension Queries ---

int32 UGridTileRegistryBase::GetMinRow() const
{
    int32 Min = TNumericLimits<int32>::Max();
    for (const FGridPosition& Pos : GetAllTilePositions())
    {
        Min = FMath::Min(Min, Pos.X);
    }
    return Min == TNumericLimits<int32>::Max() ? 0 : Min;
}

int32 UGridTileRegistryBase::GetMaxRow() const
{
    int32 Max = TNumericLimits<int32>::Min();
    for (const FGridPosition& Pos : GetAllTilePositions())
    {
        Max = FMath::Max(Max, Pos.X);
    }
    return Max == TNumericLimits<int32>::Min() ? 0 : Max;
}

int32 UGridTileRegistryBase::GetMinColumn() const
{
    int32 Min = TNumericLimits<int32>::Max();
    for (const FGridPosition& Pos : GetAllTilePositions())
    {
        Min = FMath::Min(Min, Pos.Y);
    }
    return Min == TNumericLimits<int32>::Max() ? 0 : Min;
}

int32 UGridTileRegistryBase::GetMaxColumn() const
{
    int32 Max = TNumericLimits<int32>::Min();
    for (const FGridPosition& Pos : GetAllTilePositions())
    {
        Max = FMath::Max(Max, Pos.Y);
    }
    return Max == TNumericLimits<int32>::Min() ? 0 : Max;
}

int32 UGridTileRegistryBase::GetRowCount() const
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

int32 UGridTileRegistryBase::GetColumnCount() const
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

TArray<FGridPosition> UGridTileRegistryBase::GetRowPositions(int32 RowIndex) const
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

TArray<FGridPosition> UGridTileRegistryBase::GetColumnPositions(int32 ColumnIndex) const
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
