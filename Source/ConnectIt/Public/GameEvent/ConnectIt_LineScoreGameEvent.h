// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "GameEvent/TurnBasedGameEvent.h"
#include "ConnectIt_LineScoreGameEvent.generated.h"

class AGridPieceBase;
class UGridPieceRegistryComponent;

// Owns the full ConnectIt_Event_LineScored despawn sequence: for every
// position it's handed (the scoring line minus the completing tile,
// resolved by the dispatcher), trigger that position's piece's despawn
// visual and wait for it to actually finish before finalizing its removal
// from the registry -- fixing the eager-unregister-before-the-visual gap
// for this event type. Constructed and driven by
// UConnectIt_PieceSpawnInterpreter -- see its HandleLineScoredExecute.
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
