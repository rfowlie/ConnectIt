// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/Rules/ConnectIt_TilePlaceableRule.h"
#include "UObject/Object.h"
#include "ConnectIt_UnoccupiedTilePlaceableRule.generated.h"

// Default IConnectIt_TilePlaceableRule implementation -- a tile is
// placeable if it's active and not already occupied. Ported unchanged from
// UConnectIt_BoardRequestMediator's previous hardcoded
// FConnectItBoardState::IsTileValidForPlacement check.
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CONNECTIT_API UConnectIt_UnoccupiedTilePlaceableRule : public UObject, public IConnectIt_TilePlaceableRule
{
    GENERATED_BODY()

public:

    virtual bool IsTilePlaceable_Implementation(
        const FConnectItBoardState& State, FGridPosition Position) const override;
};
