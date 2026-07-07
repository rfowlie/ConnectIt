// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/BoardManager.h"
#include "Board/BoardStateComponent.h"
#include "Board/Shift/BoardShiftComponent.h"
#include "Tile/GridTileRegistryComponent.h"


ABoardManager::ABoardManager()
{
	PrimaryActorTick.bCanEverTick = false;

	TileRegistryComponent = CreateDefaultSubobject<UGridTileRegistryComponent>(
		TEXT("GridRegistry"));
	StateComponent    = CreateDefaultSubobject<UBoardStateComponent>(
		TEXT("BoardState"));
	ShiftComponent    = CreateDefaultSubobject<UBoardShiftComponent>(
		TEXT("BoardShift"));
}

bool ABoardManager::RequestShift(const FShiftOperation Operation) const
{
	if (!IsValid(ShiftComponent)) return false;
	return ShiftComponent->RequestShift(Operation);
}

bool ABoardManager::IsShifting() const
{
	if (!IsValid(ShiftComponent)) return false;
	return ShiftComponent->IsShifting();
}
