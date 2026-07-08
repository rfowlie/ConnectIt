// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameTurnTracker.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class UNREALGAMEMECHANICS_API UGameTurnTracker : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void Reset() { Count = 0; }
	UFUNCTION(BlueprintCallable)
	void Increment() { Count++; }
	UFUNCTION(BlueprintCallable)
	int32 GetCount() const { return Count; }

protected:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Game Turn")
	int32 Count = 0;
};
