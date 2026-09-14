// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/Rules/ConnectIt_UnoccupiedTilePlaceableRule.h"


bool UConnectIt_UnoccupiedTilePlaceableRule::IsTilePlaceable_Implementation(
    const FConnectItBoardState& State, FGridPosition Position) const
{
    return State.IsTileValidForPlacement(Position);
}
