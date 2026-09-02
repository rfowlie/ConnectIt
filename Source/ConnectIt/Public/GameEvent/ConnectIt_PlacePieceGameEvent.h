// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "GameEvent/TurnBasedGameEvent.h"
#include "ConnectIt_PlacePieceGameEvent.generated.h"

class AGridPieceBase;
class AGridTileBase;
class UGridPieceRegistryComponent;
class UGridPieceSpawnInterpreter;

// Owns the full ConnectIt_Event_PiecePlaced sequence: retrieve a piece of
// the right class from the pool, initialize it (faction ownership) before
// its visual plays, position it and trigger the visual, then wait for that
// visual to actually finish before reporting done. Constructed and driven
// by UConnectIt_PieceSpawnInterpreter (the dispatcher) -- see its
// HandlePiecePlacedExecute.
UCLASS()
class CONNECTIT_API UConnectIt_PlacePieceGameEvent : public UTurnBasedGameEvent
{
    GENERATED_BODY()

public:

    // Explicit dependency injection -- called by the dispatcher right
    // after construction, before Execute(). Mirrors UTurnBasedAction::
    // InitialiseAction's shape.
    void Initialise(
        UGridPieceRegistryComponent* InPieceRegistry,
        UGridPieceSpawnInterpreter* InSpawnInterpreter,
        TSubclassOf<AGridPieceBase> InPieceClass,
        AGridTileBase* InTile,
        int32 InFactionSlot,
        FGridPosition InPosition);

protected:

    virtual void Execute_Implementation() override;

private:

    UPROPERTY()
    TObjectPtr<UGridPieceRegistryComponent> PieceRegistry = nullptr;

    UPROPERTY()
    TObjectPtr<UGridPieceSpawnInterpreter> SpawnInterpreter = nullptr;

    TSubclassOf<AGridPieceBase> PieceClass;

    UPROPERTY()
    TObjectPtr<AGridTileBase> Tile = nullptr;

    int32 FactionSlot = -1;

    FGridPosition Position;

    // The piece this event retrieved -- held so HandlePieceSpawned can
    // tell "is this firing of the shared OnPieceSpawned signal mine."
    UPROPERTY()
    TObjectPtr<AGridPieceBase> Piece = nullptr;

    UFUNCTION()
    void HandlePieceSpawned(AGridPieceBase* SpawnedPiece);
};
