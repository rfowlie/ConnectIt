// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action/Interface/TurnBasedActionNotifier.h"
#include "GameFramework/HUD.h"
#include "ConnectIt_HUD.generated.h"

/**
 *
 */
UCLASS()
class CONNECTIT_API AConnectIt_HUD : public AHUD, public ITurnBasedActionNotifier
{
	GENERATED_BODY()
};
