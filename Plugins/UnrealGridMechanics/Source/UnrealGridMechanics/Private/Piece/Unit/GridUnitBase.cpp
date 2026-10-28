// Fill out your copyright notice in the Description page of Project Settings.


#include "Piece/Unit/GridUnitBase.h"


// Sets default values
AGridUnitBase::AGridUnitBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AGridUnitBase::BeginPlay()
{
	Super::BeginPlay();
}