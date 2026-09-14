// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ConnectIt_ActionRequestMediator.generated.h"

/**
 * owned by game mode
 * listen to requests and update action availablility for players
 * should update player state which has TArray<TurnBasedActions> GetPlayerActions(PlayerController* Controller) const;
 */
UCLASS()
class CONNECTIT_API UConnectIt_ActionRequestMediator : public UObject
{
	GENERATED_BODY()
};
