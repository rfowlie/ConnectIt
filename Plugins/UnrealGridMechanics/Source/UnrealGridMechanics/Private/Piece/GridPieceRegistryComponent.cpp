// Fill out your copyright notice in the Description page of Project Settings.


#include "Piece/GridPieceRegistryComponent.h"
#include "Piece/GridPieceBase.h"
#include "Pooling/ActorPoolSubsystem.h"
#include "Subsystem/GridHoverSubsystem.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridTileRegistryComponent.h"


UGridPieceRegistryComponent::UGridPieceRegistryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


AGridPieceBase* UGridPieceRegistryComponent::GetPiece(const FGridPosition Position)
{
	AGridPieceBase* const* Found = PieceMap.Find(Position);
	AGridPieceBase* Piece = Found ? *Found : nullptr;
	if (!IsValid(Piece))
	{
		Piece = InstantiatePiece(Position);
		if (IsValid(Piece))
		{
			PieceMap.Add(Position, Piece);
		}
	}

	return Piece;
}

AGridPieceBase* UGridPieceRegistryComponent::SpawnPieceAt(
	TSubclassOf<AGridPieceBase> PieceClass, AGridTileBase* Tile)
{
	AGridPieceBase* Piece = RetrievePiece(PieceClass);
	if (IsValid(Piece))
	{
		ActivatePieceAt(Piece, Tile);
	}
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

	DeactivatePiece(Piece);
	ReleasePiece(Piece);
	PieceMap.Remove(Position);
}

AGridPieceBase* UGridPieceRegistryComponent::RetrievePiece(TSubclassOf<AGridPieceBase> PieceClass)
{
	if (!PieceClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("GridPieceRegistryComponent: RetrievePiece — PieceClass invalid"));
		return nullptr;
	}

	const auto HoverSubsystem = GetWorld()->GetSubsystem<UGridHoverSubsystem>();
	if (!IsValid(HoverSubsystem))
	{
		UE_LOG(LogTemp, Error,
			TEXT("GridPieceRegistryComponent: RetrievePiece — required sibling not resolved"));
		return nullptr;
	}

	UActorPoolSubsystem* PoolSubsystem =
		GetWorld() ? GetWorld()->GetSubsystem<UActorPoolSubsystem>() : nullptr;

	if (!IsValid(PoolSubsystem))
	{
		UE_LOG(LogTemp, Error,
			TEXT("GridPieceRegistryComponent: RetrievePiece — no UActorPoolSubsystem in world"));
		return nullptr;
	}

	// GetObjects does not activate the piece as a side effect -- that's
	// ActivatePieceAt's job, triggered only once a caller (this class's
	// own SpawnPieceAt, or a project-specific caller doing its own
	// initialization first) has had a chance to bind a completion
	// listener, so a piece whose activation completes synchronously can't
	// have that completion missed.
	TArray<AActor*> Objects = PoolSubsystem->GetObjects(PieceClass, 1);
	AGridPieceBase* Piece = Objects.IsEmpty() ? nullptr : Cast<AGridPieceBase>(Objects[0]);

	if (!IsValid(Piece))
	{
		UE_LOG(LogTemp, Error,
			TEXT("GridPieceRegistryComponent: RetrievePiece — pool retrieval failed for '%s'"),
			*PieceClass->GetName());
		return nullptr;
	}

	HoverSubsystem->RegisterPiece(Piece);
	return Piece;
}

void UGridPieceRegistryComponent::ActivatePieceAt(AGridPieceBase* Piece, AGridTileBase* Tile)
{
	if (!IsValid(Piece) || !IsValid(Tile))
	{
		UE_LOG(LogTemp, Error,
			TEXT("GridPieceRegistryComponent: ActivatePieceAt — Piece or Tile invalid"));
		return;
	}

	UActorPoolSubsystem* PoolSubsystem =
		GetWorld() ? GetWorld()->GetSubsystem<UActorPoolSubsystem>() : nullptr;

	if (!IsValid(PoolSubsystem))
	{
		UE_LOG(LogTemp, Error,
			TEXT("GridPieceRegistryComponent: ActivatePieceAt — no UActorPoolSubsystem in world"));
		return;
	}
	
	PoolSubsystem->ActivateObject(Piece);
}

void UGridPieceRegistryComponent::DeactivatePiece(AGridPieceBase* Piece) const
{
	if (!IsValid(Piece))
	{
		UE_LOG(LogTemp, Error,
			TEXT("GridPieceRegistryComponent: DeactivatePiece — Piece invalid"));
		return;
	}

	UActorPoolSubsystem* PoolSubsystem =
		GetWorld() ? GetWorld()->GetSubsystem<UActorPoolSubsystem>() : nullptr;

	if (!IsValid(PoolSubsystem))
	{
		UE_LOG(LogTemp, Error,
			TEXT("GridPieceRegistryComponent: DeactivatePiece — no UActorPoolSubsystem in world"));
		return;
	}

	PoolSubsystem->DeactivateObject(Piece);
}

void UGridPieceRegistryComponent::ReleasePiece(AGridPieceBase* Piece)
{
	if (!IsValid(Piece))
	{
		UE_LOG(LogTemp, Error,
			TEXT("GridPieceRegistryComponent: ReleasePiece — Piece invalid"));
		return;
	}

	const auto HoverSubsystem = GetWorld()->GetSubsystem<UGridHoverSubsystem>();
	if (!IsValid(HoverSubsystem))
	{
		UE_LOG(LogTemp, Error,
			TEXT("GridPieceRegistryComponent: ReleasePiece — required sibling not resolved"));
		return;
	}

	HoverSubsystem->UnregisterPiece(Piece);

	// Drop every position that mapped to this piece so a later GetPiece
	// doesn't hand back a released, deactivated actor. Direct primitive
	// callers (e.g. line-score removal) rely on this since they never call
	// DespawnPieceAt.
	for (auto It = PieceMap.CreateIterator(); It; ++It)
	{
		if (It.Value() == Piece)
		{
			It.RemoveCurrent();
		}
	}

	UActorPoolSubsystem* PoolSubsystem =
		GetWorld() ? GetWorld()->GetSubsystem<UActorPoolSubsystem>() : nullptr;

	if (IsValid(PoolSubsystem))
	{
		PoolSubsystem->ReleaseObject(Piece);
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("GridPieceRegistryComponent: ReleasePiece — no UActorPoolSubsystem in world"));
	}
}

FVector UGridPieceRegistryComponent::GridPositionToWorld(FGridPosition Position) const
{
	return FVector();
}

FGridPosition UGridPieceRegistryComponent::WorldToGridPosition(const FVector& WorldLocation) const
{
	return FGridPosition();
}

AGridTileBase* UGridPieceRegistryComponent::GetPieceAtPosition(FGridPosition Position) const
{
	return nullptr;
}

FGridPosition UGridPieceRegistryComponent::GetPositionOfPiece(const AGridPieceBase* Tile) const
{
	return FGridPosition();
}

TArray<FGridPosition> UGridPieceRegistryComponent::GetAllPositions() const
{
	return TArray<FGridPosition>();
}

TArray<FGridPosition> UGridPieceRegistryComponent::GetAllPositionsOfFaction(const int32 FactionId) const
{
	return TArray<FGridPosition>();
}

TArray<AGridPieceBase*> UGridPieceRegistryComponent::GetAllPiecesOfFaction(const int32 FactionId) const
{
	return TArray<AGridPieceBase*>();
}

void UGridPieceRegistryComponent::BeginPlay()
{
	Super::BeginPlay();
	

	if (!ResolveTileRegistry())
	{
		UE_LOG(LogTemp, Error,
			TEXT("PieceRegistryComponent: Failed to resolve UTileRegistryComponent"));
		return;
	}
}

bool UGridPieceRegistryComponent::ResolveTileRegistry()
{
	GridTileRegistryComponent = GetOwner()->FindComponentByClass<UGridTileRegistryComponent>();
	return (IsValid(GridTileRegistryComponent));
}