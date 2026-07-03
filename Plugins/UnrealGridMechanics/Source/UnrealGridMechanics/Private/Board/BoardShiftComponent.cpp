// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/BoardShiftComponent.h"
#include "Board/BoardStateComponent.h"
#include "Tile/GridTileRegistryComponent.h"
#include "UnrealGridMechanics/Public/Shift/GridMechanics_GridShiftLibrary.h"
#include "Interface/GridShiftInterface.h"
#include "Tile/GridTileBase.h"


UBoardShiftComponent::UBoardShiftComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UBoardShiftComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!ResolveComponents())
    {
        UE_LOG(LogTemp, Error,
            TEXT("BoardShiftComponent: Failed to resolve required components on %s"),
            *GetOwner()->GetName());
    }
}

bool UBoardShiftComponent::ResolveComponents()
{
    RegistryComponent = GetOwner()->FindComponentByClass<UGridTileRegistryComponent>();
    StateComponent    = GetOwner()->FindComponentByClass<UBoardStateComponent>();

    if (!IsValid(RegistryComponent))
    {
        UE_LOG(LogTemp, Error,
            TEXT("BoardShiftComponent: UGridRegistryComponent not found on owner"));
        return false;
    }

    if (!IsValid(StateComponent))
    {
        UE_LOG(LogTemp, Error,
            TEXT("BoardShiftComponent: UBoardStateComponent not found on owner"));
        return false;
    }

    return true;
}

bool UBoardShiftComponent::RequestShift(FShiftOperation Operation)
{
    if (bIsShifting)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("BoardShiftComponent: Shift requested while shift in progress — ignoring"));
        return false;
    }

    if (!IsValid(RegistryComponent) || !IsValid(StateComponent))
    {
        UE_LOG(LogTemp, Error,
            TEXT("BoardShiftComponent: Cannot shift — missing component dependencies"));
        return false;
    }

    // Get all positions in the affected row or column
    TArray<FGridPosition> AffectedPositions = Operation.Axis == EShiftAxis::Row
        ? RegistryComponent->GetRowPositions(Operation.Index)
        : RegistryComponent->GetColumnPositions(Operation.Index);

    if (AffectedPositions.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("BoardShiftComponent: No positions found for %s %d"),
            Operation.Axis == EShiftAxis::Row ? TEXT("Row") : TEXT("Column"),
            Operation.Index);
        return false;
    }

    // Compute the position remap
    ActiveResult    = UGridMechanics_GridShiftLibrary::ComputeShiftResult(AffectedPositions, Operation);
    ActiveOperation = Operation;

    if (!ActiveResult.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("BoardShiftComponent: ComputeShiftResult returned invalid result"));
        return false;
    }

    // Build world position and actor maps for instruction building
    TMap<FGridPosition, FVector>  WorldPositions = BuildWorldPositionMap(AffectedPositions);
    TMap<FGridPosition, AGridTileBase*>  TileActors     = BuildGridPositionTileMap(AffectedPositions);

    // Build per-tile instructions
    ActiveInstructions = UGridMechanics_GridShiftLibrary::BuildShiftInstructions(
        ActiveResult, TileActors, WorldPositions);

    // Send PrepareShift to all affected tiles
    for (const FTileShiftInstruction& Instruction : ActiveInstructions)
    {
        if (!IsValid(Instruction.TileActor)) continue;

        if (Instruction.TileActor->Implements<UGridShiftInterface>())
        {
            IGridShiftInterface::Execute_PrepareShift(Instruction.TileActor, Instruction);
        }
    }

    // Start the shift
    bIsShifting = true;
    ShiftAlpha  = 0.f;
    SetComponentTickEnabled(true);

    OnShiftStarted.Broadcast(ActiveOperation);

    UE_LOG(LogTemp, Log,
        TEXT("BoardShiftComponent: Shift started — %s %d by %d"),
        ActiveOperation.Axis == EShiftAxis::Row ? TEXT("Row") : TEXT("Column"),
        ActiveOperation.Index,
        ActiveOperation.GetSignedAmount());

    return true;
}

void UBoardShiftComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bIsShifting) return;

    // Advance alpha
    ShiftAlpha = FMath::Clamp(
        ShiftAlpha + (DeltaTime / FMath::Max(ShiftDuration, 0.016f)),
        0.f,
        1.f
    );

    // Broadcast current alpha to all tiles
    BroadcastAlphaToTiles(ShiftAlpha);

    // Check for completion
    if (ShiftAlpha >= 1.0f)
    {
        FinaliseShift();
    }
}

void UBoardShiftComponent::BroadcastAlphaToTiles(float Alpha)
{
    for (const FTileShiftInstruction& Instruction : ActiveInstructions)
    {
        if (!IsValid(Instruction.TileActor)) continue;

        if (Instruction.TileActor->Implements<UGridShiftInterface>())
        {
            IGridShiftInterface::Execute_ApplyShiftAlpha(Instruction.TileActor, Alpha);
        }
    }
}

void UBoardShiftComponent::FinaliseShift()
{
    // Update board state atomically
    StateComponent->ApplyShiftResult(ActiveResult);

    // Notify all tiles that shift is complete
    for (const FTileShiftInstruction& Instruction : ActiveInstructions)
    {
        if (!IsValid(Instruction.TileActor)) continue;
        if (Instruction.TileActor->Implements<UGridShiftInterface>())
        {
            IGridShiftInterface::Execute_OnShiftComplete(Instruction.TileActor);
        }
    }

    // Reset shift state
    bIsShifting = false;
    ShiftAlpha  = 0.f;
    ActiveInstructions.Empty();
    SetComponentTickEnabled(false);

    OnShiftCompleted.Broadcast(ActiveOperation);

    UE_LOG(LogTemp, Log,
        TEXT("BoardShiftComponent: Shift complete — %s %d"),
        ActiveOperation.Axis == EShiftAxis::Row ? TEXT("Row") : TEXT("Column"),
        ActiveOperation.Index);
}

TMap<FGridPosition, FVector> UBoardShiftComponent::BuildWorldPositionMap(
    const TArray<FGridPosition>& Positions) const
{
    TMap<FGridPosition, FVector> WorldPositions;
    WorldPositions.Reserve(Positions.Num());

    for (const FGridPosition& Pos : Positions)
    {
        WorldPositions.Add(Pos, RegistryComponent->GridPositionToWorld(Pos));
    }

    return WorldPositions;
}

TMap<FGridPosition, AGridTileBase*> UBoardShiftComponent::BuildGridPositionTileMap(
    const TArray<FGridPosition>& Positions) const
{
    TMap<FGridPosition, AGridTileBase*> TileActors;
    TileActors.Reserve(Positions.Num());

    for (FGridPosition Position : Positions)
    {
        if (AGridTileBase* Tile = RegistryComponent->GetTile(Position))
        {
            TileActors.Add(Position, Tile);
        }
    }

    return TileActors;
}