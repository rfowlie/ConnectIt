// Fill out your copyright notice in the Description page of Project Settings.


#include "Tile/GridTileBase.h"

#include "GridMechanics_GridLibrary.h"
#include "Subsystem/GridTrackerSubsystem.h"
#include "Subsystem/GridWorldSubsystem.h"


AGridTileBase::AGridTileBase()
{
}

void AGridTileBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (UGridWorldSubsystem* Subsystem = GetWorld()->GetSubsystem<UGridWorldSubsystem>())
	{
		Subsystem->RegisterTile(
			this, UGridMechanics_GridLibrary::CalculateGridPositionFromSize(this, 100.f));
	}	
	
	if (UGridTrackerSubsystem* Subsystem = GetWorld()->GetSubsystem<UGridTrackerSubsystem>())
	{
		Subsystem->RegisterGridTile(this);
	}
}