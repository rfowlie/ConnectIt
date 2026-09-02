// Fill out your copyright notice in the Description page of Project Settings.


#include "Tile/GridTileRegistryComponent.h"
#include "Tile/GridTileBase.h"
#include "Subsystem/GridHoverSubsystem.h"
#include "Algo/Sort.h"
#include "EngineUtils.h"


UGridTileRegistryComponent::UGridTileRegistryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGridTileRegistryComponent::BeginPlay()
{
    Super::BeginPlay();

    ResolveSubsystem();
    DiscoverTiles();

    if (IsValid(HoverSubsystem))
    {
        for (const TObjectPtr<AGridTileBase>& Tile : Tiles)
        {
            HoverSubsystem->RegisterTile(Tile);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GridTileRegistryComponent: no UGridHoverSubsystem - hover relay disabled, queries still work"));
    }

    if (UWorld* World = GetWorld())
    {
        ActorSpawnedHandle = World->AddOnActorSpawnedHandler(
            FOnActorSpawned::FDelegate::CreateUObject(this, &UGridTileRegistryComponent::HandleActorSpawned));
    }

#if WITH_EDITOR
    ValidateTileAlignment();
#endif
}

void UGridTileRegistryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

    Super::EndPlay(EndPlayReason);
}

bool UGridTileRegistryComponent::ResolveSubsystem()
{
    HoverSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UGridHoverSubsystem>() : nullptr;
    return IsValid(HoverSubsystem);
}

void UGridTileRegistryComponent::DiscoverTiles()
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

    // Deterministic order so InitialiseBoardState / row & column queries are
    // stable across runs.
    Algo::Sort(Tiles, [](const TObjectPtr<AGridTileBase>& A, const TObjectPtr<AGridTileBase>& B)
    {
        return GetNameSafe(A.Get()) < GetNameSafe(B.Get());
    });
}

void UGridTileRegistryComponent::HandleActorSpawned(AActor* SpawnedActor)
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

AGridTileBase* UGridTileRegistryComponent::GetTileAtPosition(const FGridPosition Position) const
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

FGridPosition UGridTileRegistryComponent::GetPositionOfTile(const AGridTileBase* Tile) const
{
    return WorldToGridPosition(Tile->GetActorLocation());
}

TArray<AGridTileBase*> UGridTileRegistryComponent::GetRow(const int32 RowIndex) const
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

TArray<AGridTileBase*> UGridTileRegistryComponent::GetColumn(const int32 ColumnIndex) const
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

TArray<FGridPosition> UGridTileRegistryComponent::GetAllTilePositions() const
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

TArray<AGridTileBase*> UGridTileRegistryComponent::GetAllTiles() const
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
    /*
     * TODO: this relies on tile actors to be given a GridPosition in editor
     * this is currently not happening so always misaligned
     */
    // for (const TObjectPtr<AGridTileBase>& Tile : Tiles)
    // {
    //     if (!IsValid(Tile)) continue;
    //
    //     const FVector WorldLocation  = Tile->GetActorLocation();
    //     const FGridPosition Expected = WorldToGridPosition(WorldLocation);
    //     // const FGridPosition Actual   = Tile->GetGridPosition();
    //     const FGridPosition Actual   = FGridPosition();
    //
    //     if (Expected != Actual)
    //     {
    //         UE_LOG(LogTemp, Warning,
    //             TEXT("GridRegistryComponent: Tile %s is misaligned — "
    //                  "GridPosition (%d,%d) does not match world position (%d,%d). "
    //                  "Expected GridPosition based on world location with GridSize %d."),
    //             *Tile->GetName(),
    //             Actual.X, Actual.Y,
    //             Expected.X, Expected.Y,
    //             GridSize);
    //     }
    // }
}
#endif
