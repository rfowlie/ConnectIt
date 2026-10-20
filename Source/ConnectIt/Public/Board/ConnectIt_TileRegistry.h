// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tile/GridTileRegistryBase.h"
#include "ConnectIt_TileRegistry.generated.h"

class UConnectIt_BoardStateComponent;

// ConnectIt-specific tile registry -- the plugin's UGridTileRegistryBase
// knows nothing about UConnectIt_BoardStateComponent, so board-state access
// lives here rather than in UnrealGridMechanics, which stays project-agnostic.
//
// No injection needed: UConnectIt_BoardRegistrySubsystem constructs this via
// DuplicateObject(Template, this), outered to the subsystem (itself outered
// to the UWorld) -- so GetWorld() already resolves correctly, exactly the
// precondition UConnectIt_BoardRequestMediator::GetBoardState already relies
// on for the same reason. GetBoardState() just pulls on demand rather than
// being pushed a reference at construction time.
UCLASS(Blueprintable)
class CONNECTIT_API UConnectIt_TileRegistry : public UGridTileRegistryBase
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    UConnectIt_BoardStateComponent* GetBoardState() const;

    UFUNCTION(BlueprintImplementableEvent, Category = "ConnectIt|Board")
    void HandleGameEventComplete(FGameplayTag EventTag);
};
