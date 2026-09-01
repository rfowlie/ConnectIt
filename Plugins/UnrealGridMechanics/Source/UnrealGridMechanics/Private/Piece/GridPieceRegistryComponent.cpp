// Fill out your copyright notice in the Description page of Project Settings.


#include "Piece/GridPieceRegistryComponent.h"

#include "Piece/GridPieceBase.h"
#include "Piece/GridPieceSpawnInterpreter.h"
#include "Pooling/ActorPoolSubsystem.h"
#include "Subsystem/GridWorldSubsystem.h"
#include "Tile/GridTileBase.h"
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

AGridPieceBase* UGridPieceRegistryComponent::SpawnPieceAt(
	TSubclassOf<AGridPieceBase> PieceClass, AGridTileBase* Tile)
{
	if (!PieceClass || !IsValid(Tile))
	{
		UE_LOG(LogTemp, Error,
			TEXT("GridPieceRegistryComponent: SpawnPieceAt — PieceClass or Tile invalid"));
		return nullptr;
	}

	if (!IsValid(GridSubsystem) || !IsValid(SpawnInterpreter))
	{
		UE_LOG(LogTemp, Error,
			TEXT("GridPieceRegistryComponent: SpawnPieceAt — required sibling not resolved"));
		return nullptr;
	}

	UActorPoolSubsystem* PoolSubsystem =
		GetWorld() ? GetWorld()->GetSubsystem<UActorPoolSubsystem>() : nullptr;

	if (!IsValid(PoolSubsystem))
	{
		UE_LOG(LogTemp, Error,
			TEXT("GridPieceRegistryComponent: SpawnPieceAt — no UActorPoolSubsystem in world"));
		return nullptr;
	}

	// GetObjects no longer activates the piece as a side effect -- it's
	// triggered explicitly below, after SpawnInterpreter has had a chance
	// to bind its completion listener, so a piece whose activation
	// completes synchronously can't have that completion missed.
	TArray<AActor*> Objects = PoolSubsystem->GetObjects(PieceClass, 1);
	AGridPieceBase* Piece = Objects.IsEmpty() ? nullptr : Cast<AGridPieceBase>(Objects[0]);

	if (!IsValid(Piece))
	{
		UE_LOG(LogTemp, Error,
			TEXT("GridPieceRegistryComponent: SpawnPieceAt — pool retrieval failed for '%s'"),
			*PieceClass->GetName());
		return nullptr;
	}

	GridSubsystem->RegisterPiece(Piece);
	SpawnInterpreter->SpawnPiece(Piece, Tile);
	PoolSubsystem->ActivateObject(Piece);

	return Piece;
}

void UGridPieceRegistryComponent::DespawnPieceAt(FGridPosition Position)
{
	AGridPieceBase* Piece = GetPiece(Position);
	if (!IsValid(Piece))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("GridPieceRegistryComponent: DespawnPieceAt — no piece registered at (%d,%d)"),
			Position.X, Position.Y);
		return;
	}

	if (!IsValid(GridSubsystem) || !IsValid(SpawnInterpreter))
	{
		UE_LOG(LogTemp, Error,
			TEXT("GridPieceRegistryComponent: DespawnPieceAt — required sibling not resolved"));
		return;
	}

	UActorPoolSubsystem* PoolSubsystem =
		GetWorld() ? GetWorld()->GetSubsystem<UActorPoolSubsystem>() : nullptr;

	GridSubsystem->UnregisterPiece(Piece);

	// SpawnInterpreter binds its completion listener first -- deactivation
	// is triggered explicitly after, so a piece whose deactivation
	// completes synchronously can't have that completion missed (same
	// reasoning as SpawnPieceAt's ActivateObject call). ReleaseObject
	// (pool bookkeeping -- marks Piece available for reuse) runs last,
	// after deactivation has actually been triggered, matching this
	// function's previous relative ordering.
	if (IsValid(PoolSubsystem))
	{
		SpawnInterpreter->DespawnPiece(Piece);
		PoolSubsystem->DeactivateObject(Piece);
		PoolSubsystem->ReleaseObject(Piece);
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("GridPieceRegistryComponent: DespawnPieceAt — no UActorPoolSubsystem in world"));
		SpawnInterpreter->DespawnPiece(Piece);
	}
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

	if (!ResolveSpawnInterpreter())
	{
		UE_LOG(LogTemp, Error,
			TEXT("PieceRegistryComponent: Failed to resolve UGridPieceSpawnInterpreter"));
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

bool UGridPieceRegistryComponent::ResolveSpawnInterpreter()
{
	SpawnInterpreter = GetOwner()->FindComponentByClass<UGridPieceSpawnInterpreter>();
	return IsValid(SpawnInterpreter);
}
