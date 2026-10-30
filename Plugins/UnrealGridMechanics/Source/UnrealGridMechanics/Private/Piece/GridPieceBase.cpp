// Fill out your copyright notice in the Description page of Project Settings.


#include "Piece/GridPieceBase.h"
#include "Components/PrimitiveComponent.h"
#include "GridMechanicsSettings.h"


AGridPieceBase::AGridPieceBase()
{
}

void AGridPieceBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// The tile under the piece gets the cursor, not the piece
	if (!bBlocksCursorTrace)
	{
		const ECollisionChannel Channel = UGridMechanicsSettings::GetCursorTraceChannel();
		TInlineComponentArray<UPrimitiveComponent*> Primitives(this);
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			Primitive->SetCollisionResponseToChannel(Channel, ECR_Ignore);
		}
	}
}

void AGridPieceBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Set up as a piece with no collision work in the Blueprint (the root
	// isn't there yet in the C++ constructor -- a Blueprint adds it)
	if (!bBlocksCursorTrace)
	{
		UGridMechanicsSettings::SetRootCursorResponse(this, ECR_Ignore);
	}
}

void AGridPieceBase::BeginPlay()
{
	Super::BeginPlay();

	// Pieces no longer self-register -- UGridPieceRegistryComponent
	// registers/unregisters them with UGridHoverSubsystem around pool
	// retrieve / release.
}
