// Fill out your copyright notice in the Description page of Project Settings.


#include "Piece/GridPieceBase.h"


// Sets default values
AGridPieceBase::AGridPieceBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AGridPieceBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGridPieceBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

