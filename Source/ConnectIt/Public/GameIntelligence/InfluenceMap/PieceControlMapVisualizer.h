// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InfluenceMap/GI_InfluenceMapVisualiserInterface.h"
#include "PieceControlMapVisualizer.generated.h"


/**
 * 
 */
UCLASS()
class CONNECTIT_API UPieceControlMapVisualizer : public UObject, public IGI_InfluenceMapVisualiser
{
	GENERATED_BODY()

public:

	virtual FGameplayTag GetTag() const override;
	virtual FText GetDisplayName() const override;
	virtual bool IsActive() const override;
	virtual void Activate() override;
	virtual void Deactivate() override;
	
};
