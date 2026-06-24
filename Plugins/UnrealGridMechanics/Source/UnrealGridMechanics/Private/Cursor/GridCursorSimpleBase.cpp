// Fill out your copyright notice in the Description page of Project Settings.


#include "Cursor/GridCursorSimpleBase.h"
#include "Tile/GridTileBase.h"
#include "Subsystem/GridTrackerSubsystem.h"


// Sets default values
AGridCursorSimpleBase::AGridCursorSimpleBase()
{
	PrimaryActorTick.bCanEverTick = false;
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
}

// Called when the game starts or when spawned
void AGridCursorSimpleBase::BeginPlay()
{
	Super::BeginPlay();
	
	StaticMeshComponent->SetMobility(EComponentMobility::Movable);
	StaticMeshComponent->SetAffectDistanceFieldLighting(false);
	StaticMeshComponent->SetAffectDynamicIndirectLighting(false);

	if (UGridTrackerSubsystem* Subsystem = GetWorld()->GetSubsystem<UGridTrackerSubsystem>())
	{
		Subsystem->OnGridTileHoveredStart.AddDynamic(this, &ThisClass::UpdateCursor);
	}
	
}

void AGridCursorSimpleBase::EnableCursor(bool bValue)
{
	PauseCursor(bValue);
	SetActorHiddenInGame(!bValue);
}

void AGridCursorSimpleBase::PauseCursor(bool bValue)
{
	IsEnabled = !bValue;
}

void AGridCursorSimpleBase::UpdateCursor(AGridTileBase* GridTile)
{
	if (IsValid(GridTile) && IsEnabled)
	{
		SetActorLocation(GridTile->GetActorLocation() + Cursor_ExtraHeight);
		// UE_LOG(LogTemp, Display, TEXT("Cursor Updated"));
	}	
}