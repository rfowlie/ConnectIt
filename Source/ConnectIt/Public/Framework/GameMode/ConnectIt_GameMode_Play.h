// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Interface/ConnectIt_FacadeHandlerInterface.h"
#include "Framework/Interface/ConnectIt_GameStateHandlerInterface.h"
#include "GameFramework/GameModeBase.h"
#include "ConnectIt_GameMode_Play.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class CONNECTIT_API AConnectIt_GameMode_Play : public AGameModeBase,
	public IConnectIt_GameStateHandlerInterface,
	public IConnectIt_FacadeHandlerInterface
{
	GENERATED_BODY()
	
	
};
