// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Adventure/ConnectIt_AdventureProgress.h"
#include "ConnectIt_AdventureSaveGame.generated.h"


// The Adventure save: the player's progress (FConnectItAdventureProgress) --
// completed nodes, broken seals, shards, the item inventory, seals held, the
// last-played node. Read and written only by UConnectIt_AdventureSubsystem.
UCLASS()
class CONNECTIT_API UConnectIt_AdventureSaveGame : public USaveGame
{
    GENERATED_BODY()

public:

    UPROPERTY()
    FConnectItAdventureProgress Progress;

    // Bumped when the saved data's shape changes, so an older save can be
    // read forward
    UPROPERTY()
    int32 SaveVersion = 1;
};
