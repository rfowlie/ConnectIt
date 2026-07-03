// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ConnectIt_BoardManager.generated.h"

UCLASS()
class CONNECTIT_API AConnectIt_BoardManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AConnectIt_BoardManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
