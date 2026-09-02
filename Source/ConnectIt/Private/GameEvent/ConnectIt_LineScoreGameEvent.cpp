// Fill out your copyright notice in the Description page of Project Settings.


#include "GameEvent/ConnectIt_LineScoreGameEvent.h"

#include "Piece/GridPieceBase.h"
#include "Piece/GridPieceRegistryComponent.h"
#include "Piece/GridPieceSpawnInterpreter.h"


void UConnectIt_LineScoreGameEvent::Initialise(
    UGridPieceRegistryComponent* InPieceRegistry,
    UGridPieceSpawnInterpreter* InSpawnInterpreter,
    const TArray<FGridPosition>& InPositionsToDespawn)
{
    PieceRegistry = InPieceRegistry;
    SpawnInterpreter = InSpawnInterpreter;
    PositionsToDespawn = InPositionsToDespawn;
}

void UConnectIt_LineScoreGameEvent::Execute_Implementation()
{
    if (!IsValid(PieceRegistry) || !IsValid(SpawnInterpreter))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_LineScoreGameEvent: Execute — missing required dependency"));
        Complete();
        return;
    }

    for (const FGridPosition& Position : PositionsToDespawn)
    {
        AGridPieceBase* Piece = PieceRegistry->GetPiece(Position);

        if (!IsValid(Piece))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ConnectIt_LineScoreGameEvent: Execute — no piece at (%d,%d)"),
                Position.X, Position.Y);
            continue;
        }

        PendingPieces.Add(Piece);
    }

    if (PendingPieces.IsEmpty())
    {
        Complete();
        return;
    }

    // Bind once, not per-piece -- AddDynamic isn't idempotent, a per-piece
    // bind would double-fire this handler on every broadcast.
    SpawnInterpreter->OnPieceDespawned.AddDynamic(
        this, &UConnectIt_LineScoreGameEvent::HandlePieceDespawned);

    for (const TWeakObjectPtr<AGridPieceBase>& WeakPiece : PendingPieces)
    {
        if (AGridPieceBase* Piece = WeakPiece.Get())
        {
            SpawnInterpreter->DespawnPiece(Piece); // binds Piece's own visual-completion listener
            PieceRegistry->DeactivatePiece(Piece);  // trigger, listener already bound
        }
    }
}

void UConnectIt_LineScoreGameEvent::HandlePieceDespawned(AGridPieceBase* Piece)
{
    // Not something this event is waiting on -- ignore. Either a stray
    // signal, or (see UGridPieceSpawnInterpreter's own known-limitation
    // comment) a piece whose completion already fired before it could be
    // added to PendingPieces.
    if (PendingPieces.Remove(Piece) == 0) return;

    // Only now -- once the despawn visual has actually finished -- is it
    // safe to finalize the removal. This is the fix: finalizing eagerly,
    // right after triggering deactivation, was the original bug.
    if (IsValid(PieceRegistry))
    {
        PieceRegistry->ReleasePiece(Piece);
    }

    if (!PendingPieces.IsEmpty()) return;

    if (IsValid(SpawnInterpreter))
    {
        SpawnInterpreter->OnPieceDespawned.RemoveDynamic(
            this, &UConnectIt_LineScoreGameEvent::HandlePieceDespawned);
    }

    Complete();
}
