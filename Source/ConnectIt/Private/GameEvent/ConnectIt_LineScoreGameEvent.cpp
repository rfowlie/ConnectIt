// Fill out your copyright notice in the Description page of Project Settings.


#include "GameEvent/ConnectIt_LineScoreGameEvent.h"
#include "Piece/GridPieceBase.h"
#include "Piece/GridPieceRegistryComponent.h"


void UConnectIt_LineScoreGameEvent::Initialise(
    UGridPieceRegistryComponent* InPieceRegistry,
    const TArray<FGridPosition>& InPositionsToDespawn)
{
    PieceRegistry = InPieceRegistry;
    PositionsToDespawn = InPositionsToDespawn;
}

void UConnectIt_LineScoreGameEvent::Execute_Implementation()
{
    // if (!IsValid(PieceRegistry))
    // {
    //     UE_LOG(LogTemp, Error,
    //         TEXT("ConnectIt_LineScoreGameEvent: Execute — missing required dependency"));
    //     Complete();
    //     return;
    // }
    //
    // for (const FGridPosition& Position : PositionsToDespawn)
    // {
    //     AGridPieceBase* Piece = PieceRegistry->GetPiece(Position);
    //
    //     if (!IsValid(Piece))
    //     {
    //         UE_LOG(LogTemp, Warning,
    //             TEXT("ConnectIt_LineScoreGameEvent: Execute — no piece at (%d,%d)"),
    //             Position.X, Position.Y);
    //         continue;
    //     }
    //
    //     PendingPieces.Add(Piece);
    // }
    //
    // if (PendingPieces.IsEmpty())
    // {
    //     Complete();
    // }
}

void UConnectIt_LineScoreGameEvent::HandlePieceDespawned(AGridPieceBase* Piece)
{
    // // Not something this event is waiting on -- ignore. Either a stray
    // // signal, or (see UGridPieceSpawnInterpreter's own known-limitation
    // // comment) a piece whose completion already fired before it could be
    // // added to PendingPieces.
    // if (PendingPieces.Remove(Piece) == 0) return;
    //
    // // Only now -- once the despawn visual has actually finished -- is it
    // // safe to finalize the removal. This is the fix: finalizing eagerly,
    // // right after triggering deactivation, was the original bug.
    // if (IsValid(PieceRegistry))
    // {
    //     PieceRegistry->ReleasePiece(Piece);
    // }
    //
    // if (!PendingPieces.IsEmpty()) return;
    //
    // Complete();
}
