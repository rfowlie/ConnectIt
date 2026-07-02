// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ConnectIt_PlayerData.generated.h"

/**
 * 
 */
UCLASS()
class CONNECTIT_API UConnectIt_PlayerData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConnectIt | Data")
	UObject* PlayerObject = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConnectIt | Data")
	int32 PlayerId = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConnectIt | Data")
	int32 Score = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConnectIt | Data")
	FText Name = FText();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConnectIt | Data")
	AActor* LastTileSelected = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConnectIt | Data")
	AActor* LastPiecePlaced = nullptr;

	UFUNCTION(BlueprintCallable, Category = "ConnectIt | Data")
	void AddToScore(const int32 InScore);
};
