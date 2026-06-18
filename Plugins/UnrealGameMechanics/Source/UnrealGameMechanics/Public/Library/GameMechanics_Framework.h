// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameMechanics_Framework.generated.h"

/**
 * 
 */
UCLASS()
class UNREALGAMEMECHANICS_API UGameMechanics_Framework : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "GameMechanics|Framework")
	static UGameInstance* GetGameInstance(UObject* WorldContextObject);
	
	UFUNCTION(BlueprintCallable, Category = "GameMechanics|Framework")
	static AGameModeBase* GetGameMode(UObject* WorldContextObject);
	
};
