// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "UnrealGameStructs.generated.h"


// this is useful in many different scenarios
USTRUCT(BlueprintType)
struct FGameplayTagValuePair
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value = 0.f;
	
	bool operator==(const FGameplayTagValuePair& Other) const
	{
		return Tag == Other.Tag;
	}
};

// this is useful in many different scenarios
USTRUCT(BlueprintType)
struct FGameplayTagValueProbability
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Probability = 0.f;
	
	bool operator==(const FGameplayTagValuePair& Other) const
	{
		return Tag == Other.Tag;
	}
};