// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ConnectIt_BoardRegistrySubsystem.generated.h"

class UConnectIt_TileRegistry;
class UConnectIt_PieceRegistry;
class UGridDefinition;

// The single per-world owner of the board's tile/piece registries.
// TileRegistry/PieceRegistry are level-authored, deterministic, world-scoped
// singletons -- not per-machine or per-player state -- so they belong here,
// not on AConnectIt_PlayerController (where they used to live) or any
// world-placed actor. Every machine's own world (server and each client)
// resolves its own instance of this subsystem independently, matching the
// project's existing symmetric-resolution philosophy for level-authored data
// (see UConnectIt_GameUtilityLibrary::GetLevelConfig).
//
// Since a UWorldSubsystem isn't level-placed or Blueprint-configurable the
// way an actor is, the concrete registry subclass a designer picks per
// level -- plus the GridDefinition (grid geometry) both registries share --
// live as Instanced template properties on UConnectIt_LevelConfigDataAsset
// instead. This subsystem resolves that asset once (OnWorldBeginPlay) and
// DuplicateObject()s its own per-world runtime instance from each template
// -- the template itself is never used live. See ConnectIt_LevelConfigDataAsset.h
// and this class's own .cpp for why: a UDataAsset's Instanced subobjects are
// shared, loaded-once objects, unsafe to use directly as a live per-world
// registry (wrong GetWorld() resolution, and shared mutable state across
// simultaneous worlds -- editor + PIE, or multiple PIE clients).
//
// Also owns wiring PieceRegistry to UGameEventTaskSubsystem::OnAnyTagComplete
// so its position->actor mappings stay in sync with board mutations -- see
// UConnectIt_PieceRegistry::HandleGameEventComplete.
UCLASS()
class CONNECTIT_API UConnectIt_BoardRegistrySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:

    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    UConnectIt_TileRegistry* GetTileRegistry() const { return TileRegistry; }

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    UConnectIt_PieceRegistry* GetPieceRegistry() const { return PieceRegistry; }

    // Grid geometry TileRegistry/PieceRegistry both share -- reachable here
    // for anything that doesn't already hold a registry reference but
    // needs it (e.g. GetWorld()->GetSubsystem<UConnectIt_BoardRegistrySubsystem>()
    // ->GetGridDefinition()).
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    UGridDefinition* GetGridDefinition() const { return GridDefinition; }

private:

    // Runtime-only duplicates of the level config's registry templates --
    // never author-edited directly, hence no Instanced/EditAnywhere here.
    UPROPERTY()
    TObjectPtr<UConnectIt_TileRegistry> TileRegistry;

    UPROPERTY()
    TObjectPtr<UConnectIt_PieceRegistry> PieceRegistry;

    UPROPERTY()
    TObjectPtr<UGridDefinition> GridDefinition;
};
