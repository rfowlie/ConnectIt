// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/BoardActor.h"
#include "Board/BoardStateComponent.h"
#include "Board/BoardShiftComponent.h"
#include "Tile/GridTileRegistryComponent.h"


ABoardActor::ABoardActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RegistryComponent = CreateDefaultSubobject<UGridTileRegistryComponent>(
		TEXT("GridRegistry"));
	StateComponent    = CreateDefaultSubobject<UBoardStateComponent>(
		TEXT("BoardState"));
	ShiftComponent    = CreateDefaultSubobject<UBoardShiftComponent>(
		TEXT("BoardShift"));
}

bool ABoardActor::RequestShift(const FShiftOperation Operation) const
{
	if (!IsValid(ShiftComponent)) return false;
	return ShiftComponent->RequestShift(Operation);
}

bool ABoardActor::IsShifting() const
{
	if (!IsValid(ShiftComponent)) return false;
	return ShiftComponent->IsShifting();
}
