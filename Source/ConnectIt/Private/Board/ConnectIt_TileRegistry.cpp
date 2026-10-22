// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/ConnectIt_TileRegistry.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Framework/Data/ConnectIt_WorldSettings.h"
#include "Registry/GridDefinition.h"
#include "Tile/GridTileBase.h"
#include "Engine/World.h"


UConnectIt_BoardStateComponent* UConnectIt_TileRegistry::GetBoardState() const
{
    return UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
}

TArray<AGridTileBase*> UConnectIt_TileRegistry::SpawnLayoutTiles(const TArray<FConnectItBoardLayoutCell>& Cells)
{
    TArray<AGridTileBase*> Spawned;
    Spawned.Init(nullptr, Cells.Num());

    UWorld* World = GetWorld();
    if (!World || !IsValid(GridDefinition))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_TileRegistry: SpawnLayoutTiles -- no world or no grid definition, no tiles spawned"));
        return Spawned;
    }

    const UConnectIt_LevelConfigDataAsset* LevelConfig = AConnectIt_WorldSettings::FindLevelConfig(this);
    const TSubclassOf<AGridTileBase> DefaultClass = LevelConfig ? LevelConfig->DefaultTileClass : nullptr;

    for (int32 Index = 0; Index < Cells.Num(); Index++)
    {
        const FConnectItBoardLayoutCell& Cell = Cells[Index];
        const TSubclassOf<AGridTileBase> TileClass = Cell.TileClass ? Cell.TileClass : DefaultClass;
        if (!TileClass)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ConnectIt_TileRegistry: no tile actor for (%d,%d) -- the cell names no Tile Class and the "
                     "level config has no Default Tile Class. The tile is on the board but not shown."),
                Cell.Position.X, Cell.Position.Y);
            continue;
        }

        if (AGridTileBase* Existing = GetTileAtPosition(Cell.Position))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ConnectIt_TileRegistry: (%d,%d) already has tile actor '%s' -- not spawning another"),
                Cell.Position.X, Cell.Position.Y, *Existing->GetName());
            Spawned[Index] = Existing;
            continue;
        }

        // Registers itself through the actor-spawned listener
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Spawned[Index] = World->SpawnActor<AGridTileBase>(
            TileClass, FTransform(GridDefinition->GridPositionToWorld(Cell.Position)), Params);
    }
    return Spawned;
}

TArray<AGridTileBase*> UConnectIt_TileRegistry::UnregisterTilesAt(const TArray<FGridPosition>& Positions)
{
    TArray<AGridTileBase*> Removed;
    Removed.Reserve(Positions.Num());
    for (const FGridPosition& Position : Positions)
    {
        AGridTileBase* Tile = GetTileAtPosition(Position);
        if (Tile)
        {
            UnregisterTile(Tile);
        }
        Removed.Add(Tile);
    }
    return Removed;
}
