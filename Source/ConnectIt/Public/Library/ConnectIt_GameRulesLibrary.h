// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConnectIt_GameRulesLibrary.generated.h"

/**
 * 
 */
UCLASS()
class CONNECTIT_API UConnectIt_GameRulesLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	static constexpr int32 ConnectIt_Score_Max = 100;
	
	UFUNCTION(BlueprintCallable, Category = "ConnectIt")
	static bool IsGameOver(const TArray<float>& ScoreBoard);
};
