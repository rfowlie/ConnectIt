// Fill out your copyright notice in the Description page of Project Settings.


#include "Piece/GridPieceBase.h"
#include "Subsystem/GridWorldSubsystem.h"


AGridPieceBase::AGridPieceBase()
{
}

void AGridPieceBase::BeginPlay()
{
	Super::BeginPlay();

	if (UGridWorldSubsystem* Subsystem = GetWorld()->GetSubsystem<UGridWorldSubsystem>())
	{
		Subsystem->RegisterPiece(this, GridPosition);
	}	
}
