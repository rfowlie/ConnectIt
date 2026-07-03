// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Manager/ConnectIt_BoardManager.h"


// Sets default values
AConnectIt_BoardManager::AConnectIt_BoardManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AConnectIt_BoardManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AConnectIt_BoardManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

