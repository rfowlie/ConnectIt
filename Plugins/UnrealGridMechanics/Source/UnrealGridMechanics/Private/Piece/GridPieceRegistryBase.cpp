// Fill out your copyright notice in the Description page of Project Settings.


#include "Piece/GridPieceRegistryBase.h"
#include "Piece/GridPieceBase.h"
#include "Pooling/ActorPoolSubsystem.h"
#include "Subsystem/GridHoverSubsystem.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridTileRegistryBase.h"


void UGridPieceRegistryBase::InitialiseRegistry()
{
    // TODO
}

UGridHoverSubsystem* UGridPieceRegistryBase::ResolveHoverSubsystem() const
{
    return GetWorld() ? GetWorld()->GetSubsystem<UGridHoverSubsystem>() : nullptr;
}

AGridPieceBase* UGridPieceRegistryBase::GetPiece(FGridPosition Position)
{
    if (TObjectPtr<AGridPieceBase>* Found = PieceMap.Find(Position))
    {
        return *Found;
    }

    AGridPieceBase* Piece = InstantiatePiece(Position);
    PieceMap.Add(Position, Piece);
    return Piece;
}

AGridPieceBase* UGridPieceRegistryBase::RetrievePiece(TSubclassOf<AGridPieceBase> PieceClass)
{
    if (!PieceClass)
    {
        UE_LOG(LogTemp, Error,
            TEXT("GridPieceRegistryBase: RetrievePiece — PieceClass invalid"));
        return nullptr;
    }

    UGridHoverSubsystem* HoverSubsystem = ResolveHoverSubsystem();
    if (!IsValid(HoverSubsystem))
    {
        UE_LOG(LogTemp, Error,
            TEXT("GridPieceRegistryBase: RetrievePiece — no UGridHoverSubsystem in world"));
        return nullptr;
    }

    UActorPoolSubsystem* PoolSubsystem =
        GetWorld() ? GetWorld()->GetSubsystem<UActorPoolSubsystem>() : nullptr;

    if (!IsValid(PoolSubsystem))
    {
        UE_LOG(LogTemp, Error,
            TEXT("GridPieceRegistryBase: RetrievePiece — no UActorPoolSubsystem in world"));
        return nullptr;
    }

    // GetObjects does not activate the piece as a side effect -- that's
    // ActivatePieceAt's job, triggered only after a caller has had a
    // chance to bind a completion listener, so a piece whose activation
    // completes synchronously can't have that completion missed.
    TArray<AActor*> Objects = PoolSubsystem->GetObjects(PieceClass, 1);
    AGridPieceBase* Piece = Objects.IsEmpty() ? nullptr : Cast<AGridPieceBase>(Objects[0]);

    if (!IsValid(Piece))
    {
        UE_LOG(LogTemp, Error,
            TEXT("GridPieceRegistryBase: RetrievePiece — pool retrieval failed for '%s'"),
            *PieceClass->GetName());
        return nullptr;
    }

    HoverSubsystem->RegisterPiece(Piece);
    return Piece;
}

void UGridPieceRegistryBase::ActivatePieceAt(AGridPieceBase* Piece, AGridTileBase* Tile)
{
    if (!IsValid(Piece) || !IsValid(Tile))
    {
        UE_LOG(LogTemp, Error,
            TEXT("GridPieceRegistryBase: ActivatePieceAt — Piece or Tile invalid"));
        return;
    }

    UActorPoolSubsystem* PoolSubsystem =
        GetWorld() ? GetWorld()->GetSubsystem<UActorPoolSubsystem>() : nullptr;

    if (!IsValid(PoolSubsystem))
    {
        UE_LOG(LogTemp, Error,
            TEXT("GridPieceRegistryBase: ActivatePieceAt — no UActorPoolSubsystem in world"));
        return;
    }
    
    PoolSubsystem->ActivateObject(Piece);
}

void UGridPieceRegistryBase::DeactivatePiece(AGridPieceBase* Piece) const
{
    if (!IsValid(Piece))
    {
        UE_LOG(LogTemp, Error,
            TEXT("GridPieceRegistryBase: DeactivatePiece — Piece invalid"));
        return;
    }

    UActorPoolSubsystem* PoolSubsystem =
        GetWorld() ? GetWorld()->GetSubsystem<UActorPoolSubsystem>() : nullptr;

    if (!IsValid(PoolSubsystem))
    {
        UE_LOG(LogTemp, Error,
            TEXT("GridPieceRegistryBase: DeactivatePiece — no UActorPoolSubsystem in world"));
        return;
    }

    PoolSubsystem->DeactivateObject(Piece);
}

void UGridPieceRegistryBase::ReleasePiece(AGridPieceBase* Piece)
{
    if (!IsValid(Piece))
    {
        UE_LOG(LogTemp, Error,
            TEXT("GridPieceRegistryBase: ReleasePiece — Piece invalid"));
        return;
    }

    UGridHoverSubsystem* HoverSubsystem = ResolveHoverSubsystem();
    if (!IsValid(HoverSubsystem))
    {
        UE_LOG(LogTemp, Error,
            TEXT("GridPieceRegistryBase: ReleasePiece — no UGridHoverSubsystem in world"));
        return;
    }

    HoverSubsystem->UnregisterPiece(Piece);

    // Drop every position that mapped to this piece so a later GetPiece
    // doesn't hand back a released, deactivated actor.
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
            TEXT("GridPieceRegistryBase: ReleasePiece — no UActorPoolSubsystem in world"));
    }
}

AGridPieceBase* UGridPieceRegistryBase::SpawnPieceAt(TSubclassOf<AGridPieceBase> PieceClass, AGridTileBase* Tile)
{
    AGridPieceBase* Piece = RetrievePiece(PieceClass);
    if (IsValid(Piece))
    {
        ActivatePieceAt(Piece, Tile);
    }
    return Piece;
}

void UGridPieceRegistryBase::DespawnPieceAt(FGridPosition Position)
{
    AGridPieceBase* Piece = GetPiece(Position);
    if (!IsValid(Piece))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GridPieceRegistryBase: DespawnPieceAt — no piece registered at (%d,%d)"),
            Position.X, Position.Y);
        return;
    }

    DeactivatePiece(Piece);
    ReleasePiece(Piece);
    PieceMap.Remove(Position);
}
