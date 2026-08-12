// Fill out your copyright notice in the Description page of Project Settings.


#include "Cursor/GridCursorSimpleBase.h"
#include "Tile/GridTileBase.h"


AGridCursorSimpleBase::AGridCursorSimpleBase()
{
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
}

void AGridCursorSimpleBase::BeginPlay()
{
	Super::BeginPlay();

	StaticMeshComponent->SetMobility(EComponentMobility::Movable);
	StaticMeshComponent->SetAffectDistanceFieldLighting(false);
	StaticMeshComponent->SetAffectDynamicIndirectLighting(false);
}

void AGridCursorSimpleBase::UpdateCursor_Implementation(AGridTileBase* Tile)
{
	if (!IsValid(Tile)) return;

	SetActorLocation(Tile->GetActorLocation() + Cursor_ExtraHeight);
}
