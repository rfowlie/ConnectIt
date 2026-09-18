// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Subsystem/ConnectIt_BoardRegistrySubsystem.h"

#include "Board/ConnectIt_PieceRegistry.h"
#include "Board/ConnectIt_TileRegistry.h"
#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "GameEvent/GameEventTaskSubsystem.h"
#include "Piece/GridPieceRegistryBase.h"
#include "Registry/GridDefinition.h"
#include "Tile/GridTileRegistryBase.h"
#include "UObject/UObjectGlobals.h"


void UConnectIt_BoardRegistrySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    const UConnectIt_LevelConfigDataAsset* LevelConfig =
        UConnectIt_GameUtilityLibrary::GetLevelConfig(this);

    if (!IsValid(LevelConfig))
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectIt_BoardRegistrySubsystem: "
            "OnWorldBeginPlay — no ConnectIt_LevelConfigDataAsset found for "
            "the current level"));
        return;
    }

    // DuplicateObject, not a direct assignment -- LevelConfig's templates
    // live inside a loaded asset (shared, loaded once per session), not
    // per-world objects. Outering each duplicate to `this` gives it a
    // working GetWorld() (UWorldSubsystem -> UWorld) unique to this world,
    // which is what InitialiseRegistry()'s internal TActorIterator/
    // AddOnActorSpawnedHandler need to scope correctly under multiple
    // simultaneous worlds (editor + PIE, or several PIE clients).
    //
    // GridDefinition first -- TileRegistry/PieceRegistry's own
    // InitialiseRegistry() (via DiscoverExisting) needs it already assigned.
    if (IsValid(LevelConfig->GridDefinition))
    {
        GridDefinition = DuplicateObject<UGridDefinition>(LevelConfig->GridDefinition, this);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectIt_BoardRegistrySubsystem: "
            "OnWorldBeginPlay — level config has no GridDefinition template set"));
    }

    if (IsValid(LevelConfig->TileRegistry))
    {
        TileRegistry = DuplicateObject<UConnectIt_TileRegistry>(LevelConfig->TileRegistry, this);
        TileRegistry->GridDefinition = GridDefinition;
        TileRegistry->InitialiseRegistry();
        
        if (UGameEventTaskSubsystem* GameEventSubsystem = InWorld.GetSubsystem<UGameEventTaskSubsystem>())
        {
            GameEventSubsystem->OnAnyTagComplete.AddDynamic(
                TileRegistry, &UConnectIt_TileRegistry::HandleGameEventComplete);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("ConnectIt_BoardRegistrySubsystem: "
                "OnWorldBeginPlay — no UGameEventTaskSubsystem, TileRegistry "
                "mappings will not stay in sync"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectIt_BoardRegistrySubsystem: "
            "OnWorldBeginPlay — level config has no TileRegistry template set"));
    }

    // NOTE pieces register after as it will likely need info from piece registry
    if (IsValid(LevelConfig->PieceRegistry))
    {
        PieceRegistry = DuplicateObject<UConnectIt_PieceRegistry>(LevelConfig->PieceRegistry, this);
        PieceRegistry->GridDefinition = GridDefinition;
        PieceRegistry->InitialiseRegistry();
        
        if (UGameEventTaskSubsystem* GameEventSubsystem = InWorld.GetSubsystem<UGameEventTaskSubsystem>())
        {
            GameEventSubsystem->OnAnyTagComplete.AddDynamic(
                PieceRegistry, &UConnectIt_PieceRegistry::HandleGameEventComplete);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("ConnectIt_BoardRegistrySubsystem: "
                "OnWorldBeginPlay — no UGameEventTaskSubsystem, PieceRegistry "
                "mappings will not stay in sync"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectIt_BoardRegistrySubsystem: "
           "OnWorldBeginPlay — level config has no PieceRegistry template set"));
    }
    
    // No error log if unset -- InitialiseRegistry() is still a no-op stub
    // on UGridPieceRegistryBase, so not every level needs one authored yet.
}

void UConnectIt_BoardRegistrySubsystem::Deinitialize()
{
    if (IsValid(TileRegistry))
    {
        if (UGameEventTaskSubsystem* GameEventSubsystem =
            GetWorld() ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>() : nullptr)
        {
            GameEventSubsystem->OnAnyTagComplete.RemoveDynamic(
                TileRegistry, &UConnectIt_TileRegistry::HandleGameEventComplete);
        }
        
        TileRegistry->ShutdownRegistry();
    }

    if (IsValid(PieceRegistry))
    {
        if (UGameEventTaskSubsystem* GameEventSubsystem =
            GetWorld() ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>() : nullptr)
        {
            GameEventSubsystem->OnAnyTagComplete.RemoveDynamic(
                PieceRegistry, &UConnectIt_PieceRegistry::HandleGameEventComplete);
        }

        PieceRegistry->ShutdownRegistry();
    }

    Super::Deinitialize();
}
