// Fill out your copyright notice in the Description page of Project Settings.


#include "Tile/GridTileBase.h"
#include "GameplayTagContainer.h"
#include "GridMechanics_GridLibrary.h"
#include "Subsystem/GridTrackerSubsystem.h"
#include "Hover/GridHoverComponent.h"


AGridTileBase::AGridTileBase()
{
	HoverComponent = CreateDefaultSubobject<UGridHoverComponent>(TEXT("HoverComponent"));
}

void AGridTileBase::BeginPlay()
{
	Super::BeginPlay();

	// Tiles no longer self-register -- UGridTileRegistryComponent discovers
	// and registers them with UGridHoverSubsystem.

	// if (UGridTrackerSubsystem* Subsystem = GetWorld()->GetSubsystem<UGridTrackerSubsystem>())
	// {
	// 	Subsystem->RegisterGridTile(this);
	// }
}

void AGridTileBase::SendGameplayTag_Implementation(FGameplayTag StateTag)
{
}