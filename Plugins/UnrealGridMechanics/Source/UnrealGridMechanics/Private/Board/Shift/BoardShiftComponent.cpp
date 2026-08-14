// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/Shift/BoardShiftComponent.h"
#include "Board/BoardStateComponentBase.h"
#include "Board/Shift/GridMechanics_GridShiftLibrary.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridTileRegistryComponent.h"
#include "Interface/GridShiftInterface.h"


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
    StateComponent    = GetOwner()->FindComponentByClass<UBoardStateComponentBase>();

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

FShiftResult UBoardShiftComponent::ComputeShift(const FShiftOperation& Operation) const
{
    if (!IsValid(RegistryComponent))
    {
        UE_LOG(LogTemp, Error,
            TEXT("BoardShiftComponent: ComputeShift — missing RegistryComponent"));
        return FShiftResult();
    }

    // Get all positions in the affected row or column
    const TArray<FGridPosition> AffectedPositions = Operation.Axis == EShiftAxis::Row
        ? RegistryComponent->GetRowPositions(Operation.Index)
        : RegistryComponent->GetColumnPositions(Operation.Index);

    if (AffectedPositions.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("BoardShiftComponent: No positions found for %s %d"),
            Operation.Axis == EShiftAxis::Row ? TEXT("Row") : TEXT("Column"),
            Operation.Index);
        return FShiftResult();
    }

    return UGridMechanics_GridShiftLibrary::ComputeShiftResult(AffectedPositions, Operation);
}

bool UBoardShiftComponent::PlayShiftAnimation(const FShiftOperation& Operation, const FShiftResult& Result)
{
    if (bIsShifting)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("BoardShiftComponent: PlayShiftAnimation requested while a shift animation is already in progress — ignoring"));
        return false;
    }

    if (!IsValid(RegistryComponent) || !IsValid(StateComponent))
    {
        UE_LOG(LogTemp, Error,
            TEXT("BoardShiftComponent: Cannot animate shift — missing component dependencies"));
        return false;
    }

    if (!Result.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("BoardShiftComponent: PlayShiftAnimation given an invalid FShiftResult"));
        return false;
    }

    ActiveOperation = Operation;
    ActiveResult    = Result;

    // Get all positions in the affected row or column -- needed to build
    // world position/tile actor maps for the instructions below
    const TArray<FGridPosition> AffectedPositions = Operation.Axis == EShiftAxis::Row
        ? RegistryComponent->GetRowPositions(Operation.Index)
        : RegistryComponent->GetColumnPositions(Operation.Index);

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

    // Start the animation
    bIsShifting = true;
    ShiftAlpha  = 0.f;
    SetComponentTickEnabled(true);

    OnShiftStarted.Broadcast(ActiveOperation);

    UE_LOG(LogTemp, Log,
        TEXT("BoardShiftComponent: Shift animation started — %s %d by %d"),
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
    // Notify all tiles that animation is complete
    for (const FTileShiftInstruction& Instruction : ActiveInstructions)
    {
        if (!IsValid(Instruction.TileActor)) continue;
        if (Instruction.TileActor->Implements<UGridShiftInterface>())
        {
            IGridShiftInterface::Execute_OnShiftComplete(Instruction.TileActor);
        }
    }

    // Reset shift state
    bIsShifting         = false;
    ShiftAlpha          = 0.f;
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
        if (AGridTileBase* Tile = RegistryComponent->GetTileAtPosition(Position))
        {
            TileActors.Add(Position, Tile);
        }
    }

    return TileActors;
}