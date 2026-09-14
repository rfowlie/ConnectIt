// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "GameEvent/TurnBasedGameEvent.h"
#include "ConnectIt_LineScoreGameEvent.generated.h"

class AGridPieceBase;
class UGridPieceRegistryComponent;

// Meant to own the full ConnectIt_Event_LineScored despawn sequence: for
// every position it's handed (the scoring line minus the completing tile),
// trigger that position's piece's despawn visual and wait for it to
// actually finish before finalizing its removal from the registry --
// fixing the eager-unregister-before-the-visual gap for this event type.
// Meant to be constructed and driven by UConnectIt_BoardRequestMediator's
// CreateGameEventsFromBoardUpdate/ExecuteGameEvents (the interpreter that
// used to dispatch this has been removed).
//
// NOTE: as of this writing, Execute_Implementation's body and
// HandlePieceDespawned's body are both fully commented out -- this class
// currently does nothing when run. The sequence described above is the
// intended behavior once that rewiring lands, not a description of what
// happens today.
UCLASS()
class CONNECTIT_API UConnectIt_LineScoreGameEvent : public UTurnBasedGameEvent
{
    GENERATED_BODY()

public:

    void Initialise(
        UGridPieceRegistryComponent* InPieceRegistry,
        const TArray<FGridPosition>& InPositionsToDespawn);

protected:

    virtual void Execute_Implementation() override;

private:

    UPROPERTY()
    TObjectPtr<UGridPieceRegistryComponent> PieceRegistry = nullptr;

    TArray<FGridPosition> PositionsToDespawn;

    // Pieces this event is waiting on -- populated in Execute_Implementation,
    // drained (and each one's removal finalized) in HandlePieceDespawned as
    // its own visual-completion signal fires.
    TArray<TWeakObjectPtr<AGridPieceBase>> PendingPieces;

    UFUNCTION()
    void HandlePieceDespawned(AGridPieceBase* Piece);
};
