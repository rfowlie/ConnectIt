// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ConnectIt_Structs.h"
#include "GridMechanicsBaseStructs.h"
#include "ConnectIt_TilePlaceableRule.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable, BlueprintType)
class UConnectIt_TilePlaceableRule : public UInterface
{
    GENERATED_BODY()
};

// Pluggable placement-validity strategy -- whether a piece may be placed at
// Position at all, before any mutation happens. Assigned via
// UConnectIt_BoardRules::TilePlaceableRule (TObjectPtr<UObject>, EditAnywhere
// Instanced), mirroring IConnectIt_ScoringRule/IConnectIt_WinCondition's
// precedent. UConnectIt_UnoccupiedTilePlaceableRule (active-and-unoccupied,
// today's previously-hardcoded behaviour) is the default implementation;
// other reasons a tile can't be placed on (e.g. reserved for a different
// faction, temporarily locked) can be added as new implementers without
// touching UConnectIt_BoardRequestMediator.
class CONNECTIT_API IConnectIt_TilePlaceableRule
{
    GENERATED_BODY()

public:

    // Read-only check against the board state as it stands before this
    // placement -- implementations must not mutate State. Called from
    // UConnectIt_BoardRequestMediator::HandlePlacePieceRequest before any
    // tile data is written.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt|Placement")
    bool IsTilePlaceable(const FConnectItBoardState& State, FGridPosition Position) const;
};
