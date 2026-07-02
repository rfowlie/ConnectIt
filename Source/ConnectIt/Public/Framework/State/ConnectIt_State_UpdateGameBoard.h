// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "States/GameMechanicsStateSimple.h"
#include "UObject/Object.h"
#include "ConnectIt_State_UpdateGameBoard.generated.h"

class UConnectIt_GameFacade;
class UConnectIt_GameViewModel;

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class CONNECTIT_API UConnectIt_State_UpdateGameBoard : public UGameMechanicsStateSimple
{
	GENERATED_BODY()

public:
	static UConnectIt_State_UpdateGameBoard* Create(UObject* Outer, UConnectIt_GameFacade* InGameFacade, UConnectIt_GameViewModel* InViewModel);

protected:
	UPROPERTY()
	UConnectIt_GameViewModel* GameViewModel = nullptr;
	UPROPERTY()
	UConnectIt_GameFacade* GameFacade = nullptr;
};
