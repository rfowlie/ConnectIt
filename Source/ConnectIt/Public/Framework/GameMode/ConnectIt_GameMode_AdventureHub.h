// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ConnectIt_GameMode_AdventureHub.generated.h"

class UConnectIt_AdventureHubComponent;


// The Adventure hub maps' game mode: no match. It carries the hub
// (UConnectIt_AdventureHubComponent: the map's nodes, their states from the
// player's progress), so the hub exists only in hub maps. Set it as a hub
// map's game mode (World Settings) -- a Blueprint subclass for the hub's
// settings and its listener.
UCLASS(Blueprintable)
class CONNECTIT_API AConnectIt_GameMode_AdventureHub : public AGameModeBase
{
    GENERATED_BODY()

public:

    AConnectIt_GameMode_AdventureHub();

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure Hub")
    UConnectIt_AdventureHubComponent* GetAdventureHub() const { return AdventureHub; }

private:

    UPROPERTY(VisibleAnywhere, Category = "Adventure Hub")
    TObjectPtr<UConnectIt_AdventureHubComponent> AdventureHub;
};
