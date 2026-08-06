// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InfluenceMap/GI_FloatGridDataProviderInterface.h"
#include "PieceControlInfluenceMap.generated.h"

class UConnectIt_BoardStateComponent;
/**
 * 
 */
UCLASS()
class CONNECTIT_API UPieceControlInfluenceMap : public UObject, public IGI_FloatGridDataProviderInterface
{
	GENERATED_BODY()

public:
	void SetUpdate(UConnectIt_BoardStateComponent* InBoardStateComponent);
	virtual float GetData(FIntPoint InPoint) const override;
	// int32 GetData(FGridPosition InGridPosition);

protected:
	UPROPERTY()
	TMap<FIntPoint, int32> InfluenceMap;
	
	UFUNCTION()
	void Update();

	UPROPERTY()
	UConnectIt_BoardStateComponent* BoardStateComponent = nullptr;
};
