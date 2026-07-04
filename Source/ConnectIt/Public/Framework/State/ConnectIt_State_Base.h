// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "State/GameMechanicsStateSimple.h"
#include "State/GameStateHandlerInterface.h"
#include "ConnectIt_State_Base.generated.h"

class UConnectIt_GameFacade;
class UConnectIt_GameViewModel;

/**
 * 
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class CONNECTIT_API UConnectIt_State_Base : public UGameMechanicsStateSimple, public IGameStateHandlerInterface
{
	GENERATED_BODY()

	virtual FGameplayTag GetGameStateTag_Implementation() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game State")
	FGameplayTag GameStateTag;
	
	UPROPERTY(BlueprintReadOnly)
	UConnectIt_GameViewModel* GameViewModel = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	UConnectIt_GameFacade* GameFacade = nullptr;
};
