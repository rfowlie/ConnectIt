// Fill out your copyright notice in the Description page of Project Settings.


#include "Piece/GridPieceRegistryComponent.h"

#include "Piece/GridPieceBase.h"
#include "Subsystem/GridWorldSubsystem.h"
#include "Tile/GridTileRegistryComponent.h"


UGridPieceRegistryComponent::UGridPieceRegistryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
}


AGridPieceBase* UGridPieceRegistryComponent::GetPiece(FGridPosition Position) const
{
	if (!IsValid(GridSubsystem) || !IsValid(GridTileRegistryComponent)) return nullptr;
	// if tile does not exist at position then piece should not either
	if (!GridTileRegistryComponent->GetTileAtPosition(Position)) return nullptr;
	for (const auto Piece : GridSubsystem->GetAllPieces())
	{
		if (Position == GridTileRegistryComponent->WorldToGridPosition(
			Piece->GetActorLocation()))
		{
			return Piece;
		}
	}
	return nullptr;
}

void UGridPieceRegistryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!ResolveSubsystem())
	{
		UE_LOG(LogTemp, Error,
			TEXT("PieceRegistryComponent: Failed to resolve UGridWorldSubsystem"));
		return;
	}

	if (!ResolveTileRegistry())
	{
		UE_LOG(LogTemp, Error,
			TEXT("PieceRegistryComponent: Failed to resolve UTileRegistryComponent"));
		return;
	}
}

bool UGridPieceRegistryComponent::ResolveSubsystem()
{
	GridSubsystem = GetWorld()->GetSubsystem<UGridWorldSubsystem>();
	return IsValid(GridSubsystem);
}

bool UGridPieceRegistryComponent::ResolveTileRegistry()
{
	GridTileRegistryComponent = GetOwner()->FindComponentByClass<UGridTileRegistryComponent>();
	return (IsValid(GridTileRegistryComponent));
}

