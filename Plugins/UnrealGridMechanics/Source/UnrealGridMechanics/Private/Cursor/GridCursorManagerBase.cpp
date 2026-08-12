// Fill out your copyright notice in the Description page of Project Settings.


#include "Cursor/GridCursorManagerBase.h"
#include "Tile/GridTileBase.h"
#include "Subsystem/GridWorldSubsystem.h"


AGridCursorManagerBase::AGridCursorManagerBase()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AGridCursorManagerBase::BeginPlay()
{
    Super::BeginPlay();

    if (UGridWorldSubsystem* GridSubsystem = GetGridSubsystem())
    {
        GridSubsystem->OnGridTileHoverChanged.AddDynamic(
            this, &AGridCursorManagerBase::HandleGridTileHoverChanged);
    }
}

void AGridCursorManagerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UGridWorldSubsystem* GridSubsystem = GetGridSubsystem())
    {
        GridSubsystem->OnGridTileHoverChanged.RemoveDynamic(
            this, &AGridCursorManagerBase::HandleGridTileHoverChanged);
    }

    Super::EndPlay(EndPlayReason);
}

void AGridCursorManagerBase::EnableCursor(bool bValue)
{
    PauseCursor(!bValue);
    SetActorHiddenInGame(!bValue);
}

void AGridCursorManagerBase::PauseCursor(bool bValue)
{
    bIsEnabled = !bValue;
}

void AGridCursorManagerBase::HandleGridTileHoverChanged(AGridTileBase* Tile)
{
    if (!bIsEnabled) return;
    UpdateCursor(Tile);
}

void AGridCursorManagerBase::UpdateCursor_Implementation(AGridTileBase* Tile)
{
    // Base is a pure contract -- subclasses provide the visual
}

UGridWorldSubsystem* AGridCursorManagerBase::GetGridSubsystem() const
{
    const UWorld* World = GetWorld();
    return IsValid(World) ? World->GetSubsystem<UGridWorldSubsystem>() : nullptr;
}
