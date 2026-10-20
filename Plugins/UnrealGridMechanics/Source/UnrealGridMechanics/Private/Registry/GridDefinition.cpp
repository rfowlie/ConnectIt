// Fill out your copyright notice in the Description page of Project Settings.


#include "Registry/GridDefinition.h"

// --- Position Conversion ---
// Moved verbatim from UGridTileRegistryBase -- no behavior change, pure
// relocation. See the class comment in GridDefinition.h for why.

FVector UGridDefinition::GridPositionToWorld_Implementation(FGridPosition Position) const
{
    // The ActorComponent version anchors Z to GetOwner()->GetActorLocation().Z --
    // a plain UObject has no equivalent without an explicit owner reference
    // threaded through, which this deliberately doesn't add. Flat Z=0 here;
    // a concrete subclass with real owner/board-plane knowledge can override
    // this whole function to restore that behavior.
    return FVector(
        Position.X * GridSize,
        Position.Y * GridSize,
        0.f);
}

FGridPosition UGridDefinition::WorldToGridPosition_Implementation(const FVector& WorldLocation) const
{
    return FGridPosition(
        FMath::RoundToInt32(WorldLocation.X / GridSize),
        FMath::RoundToInt32(WorldLocation.Y / GridSize));
}
