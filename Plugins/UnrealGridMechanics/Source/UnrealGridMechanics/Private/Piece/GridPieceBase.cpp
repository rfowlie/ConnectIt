// Fill out your copyright notice in the Description page of Project Settings.


#include "Piece/GridPieceBase.h"


AGridPieceBase::AGridPieceBase()
{
}

void AGridPieceBase::BeginPlay()
{
	Super::BeginPlay();

	// Pieces no longer self-register -- UGridPieceRegistryComponent
	// registers/unregisters them with UGridHoverSubsystem around pool
	// retrieve / release.
}
