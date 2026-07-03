// Fill out your copyright notice in the Description page of Project Settings.


#include "Shift/GridMechanics_GridShiftLibrary.h"


FShiftResult UGridMechanics_GridShiftLibrary::ComputeShiftResult(
    const TArray<FGridPosition>& InPositions,
    const FShiftOperation& Operation)
{
    FShiftResult Result;

    if (InPositions.IsEmpty()) return Result;

    // Determine bounds of the row or column being shifted
    int32 MinIndex = TNumericLimits<int32>::Max();
    int32 MaxIndex = TNumericLimits<int32>::Min();

    for (const FGridPosition& Pos : InPositions)
    {
        const int32 Index = Operation.Axis == EShiftAxis::Row ? Pos.Y : Pos.X;
        MinIndex = FMath::Min(MinIndex, Index);
        MaxIndex = FMath::Max(MaxIndex, Index);
    }

    // Compute new position for each tile in the row or column
    for (const FGridPosition& OldPos : InPositions)
    {
        const FGridPosition NewPos = ComputeShiftedPosition(
            OldPos, Operation, MinIndex, MaxIndex);

        Result.PositionRemap.Add(OldPos, NewPos);

        if (IsWrapping(OldPos, Operation, MinIndex, MaxIndex))
        {
            Result.WrappingPositions.Add(OldPos);
        }
    }

    return Result;
}

FGridPosition UGridMechanics_GridShiftLibrary::ComputeShiftedPosition(
    const FGridPosition& Position,
    const FShiftOperation& Operation,
    const int32 MinIndex,
    const int32 MaxIndex)
{
    const int32 RangeSize    = MaxIndex - MinIndex + 1;
    const int32 SignedAmount = Operation.GetSignedAmount();

    if (Operation.Axis == EShiftAxis::Row)
    {
        // Shifting along Y axis (columns within a row)
        // Normalise to zero base, apply shift with wrap, restore offset
        const int32 NormalisedY  = Position.Y - MinIndex;
        const int32 ShiftedY     = ((NormalisedY + SignedAmount) % RangeSize + RangeSize) % RangeSize;
        const int32 RestoredY    = ShiftedY + MinIndex;
        return FGridPosition(Position.X, RestoredY);
    }
    else
    {
        // Shifting along X axis (rows within a column)
        const int32 NormalisedX  = Position.X - MinIndex;
        const int32 ShiftedX     = ((NormalisedX + SignedAmount) % RangeSize + RangeSize) % RangeSize;
        const int32 RestoredX    = ShiftedX + MinIndex;
        return FGridPosition(RestoredX, Position.Y);
    }
}

bool UGridMechanics_GridShiftLibrary::IsWrapping(
    const FGridPosition& Position,
    const FShiftOperation& Operation,
    const int32 MinIndex,
    const int32 MaxIndex)
{
    const int32 SignedAmount = Operation.GetSignedAmount();

    if (Operation.Axis == EShiftAxis::Row)
    {
        const int32 NewY = Position.Y + SignedAmount;
        return NewY < MinIndex || NewY > MaxIndex;
    }
    else
    {
        const int32 NewX = Position.X + SignedAmount;
        return NewX < MinIndex || NewX > MaxIndex;
    }
}

TArray<FTileShiftInstruction> UGridMechanics_GridShiftLibrary::BuildShiftInstructions(
    const FShiftResult& Result,
    const TMap<FGridPosition, AGridTileBase*>& TileActors,
    const TMap<FGridPosition, FVector>& WorldPositions)
{
    TArray<FTileShiftInstruction> Instructions;
    Instructions.Reserve(Result.PositionRemap.Num());

    for (const auto& [OldPos, NewPos] : Result.PositionRemap)
    {
        AGridTileBase* const* TileActor       = TileActors.Find(OldPos);
        const FVector* StartWorldPos   = WorldPositions.Find(OldPos);
        const FVector* EndWorldPos     = WorldPositions.Find(NewPos);

        if (!TileActor || !StartWorldPos || !EndWorldPos) continue;

        FTileShiftInstruction Instruction;
        Instruction.TileActor          = *TileActor;
        Instruction.StartWorldPosition = *StartWorldPos;
        Instruction.EndWorldPosition   = *EndWorldPos;
        Instruction.FromGridPosition   = OldPos;
        Instruction.ToGridPosition     = NewPos;
        Instruction.bIsWrapping        = Result.WrappingPositions.Contains(OldPos);

        Instructions.Add(MoveTemp(Instruction));
    }

    return Instructions;
}
