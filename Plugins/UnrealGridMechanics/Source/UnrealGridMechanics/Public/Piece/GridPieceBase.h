// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "GameFramework/Actor.h"
#include "GridPieceBase.generated.h"

/*
 * An actor that is considered a piece but does not use GAS
 */
UCLASS(BlueprintType, Blueprintable)
class UNREALGRIDMECHANICS_API AGridPieceBase : public AActor
{
	GENERATED_BODY()

public:
	AGridPieceBase();

	UFUNCTION(BlueprintPure, Category = "Grid")
	FGridPosition GetGridPosition() const { return GridPosition; }
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	FGridPosition GridPosition;

	
	
};
