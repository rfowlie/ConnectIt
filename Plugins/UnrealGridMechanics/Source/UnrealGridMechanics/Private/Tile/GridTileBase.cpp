// Fill out your copyright notice in the Description page of Project Settings.


#include "Tile/GridTileBase.h"
#include "GameplayTagContainer.h"
#include "GridMechanics_GridLibrary.h"
#include "Hover/GridHoverComponent.h"


AGridTileBase::AGridTileBase()
{
	HoverComponent = CreateDefaultSubobject<UGridHoverComponent>(TEXT("HoverComponent"));
}

void AGridTileBase::BeginPlay()
{
	Super::BeginPlay();

	// The deprecated actor delegate, kept fed for one release
	if (IsValid(HoverComponent))
	{
		HoverComponent->OnHoverBegin.AddDynamic(this, &ThisClass::ForwardHoverBegin);
	}

	// Tiles don't self-register -- UGridTileRegistryComponent discovers and
	// registers them with UGridHoverSubsystem.
}

void AGridTileBase::ForwardHoverBegin(UGridHoverComponent* Component)
{
	OnGridTileBeginCursorOver.Broadcast(this);
}

void AGridTileBase::SendGameplayTag_Implementation(FGameplayTag StateTag)
{
}