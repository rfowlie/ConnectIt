// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Piece/GridPieceRegistryComponent.h"
#include "ConnectIt_PieceRegistryComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CONNECTIT_API UConnectIt_PieceRegistryComponent : public UGridPieceRegistryComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UConnectIt_PieceRegistryComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
