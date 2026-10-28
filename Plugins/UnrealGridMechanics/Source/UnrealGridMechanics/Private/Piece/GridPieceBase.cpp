// Fill out your copyright notice in the Description page of Project Settings.


#include "Piece/GridPieceBase.h"
#include "Components/PrimitiveComponent.h"


AGridPieceBase::AGridPieceBase()
{
}

void AGridPieceBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// The tile under the piece gets the cursor, not the piece
	if (!bBlocksCursorTrace)
	{
		TInlineComponentArray<UPrimitiveComponent*> Primitives(this);
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			Primitive->SetCollisionResponseToChannel(CursorTraceChannel, ECR_Ignore);
		}
	}
}

void AGridPieceBase::BeginPlay()
{
	Super::BeginPlay();

	// Pieces no longer self-register -- UGridPieceRegistryComponent
	// registers/unregisters them with UGridHoverSubsystem around pool
	// retrieve / release.
}
