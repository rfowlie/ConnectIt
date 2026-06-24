// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UtilityScoreHandler.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UUtilityScoreHandler : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UNREALGAMEINTELLIGENCE_API IUtilityScoreHandler
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GameIntelligence | Utility")
	float GetUtilityScore();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GameIntelligence | Utility")
	void SetUtilityScore(const float InScore);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GameIntelligence | Utility")
	void ShowUtilityScore();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GameIntelligence | Utility")
	void HideUtilityScore();
};
