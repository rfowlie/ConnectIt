// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	// Sets default values for this actor's properties
	AGridPieceBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
