// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConnectIt_GridRulesLibrary.generated.h"

/**
 * 
 */
UCLASS()
class CONNECTIT_API UConnectIt_GridRulesLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	static constexpr float ConnectIt_Grid_Size_Default = 100.f;
	
	UFUNCTION(BlueprintCallable, Category = "ConnectIt | Library | Grid")
	void Test();
};
