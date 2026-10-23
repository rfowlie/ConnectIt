// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/ConnectIt_BoardLayoutCell.h"


FConnectItBoardLayoutDiff FConnectItBoardLayoutDiff::Between(
    TConstArrayView<FGridPosition> CurrentPositions, TConstArrayView<FConnectItBoardLayoutCell> Cells)
{
    TSet<FGridPosition> Current;
    Current.Reserve(CurrentPositions.Num());
    for (const FGridPosition& Position : CurrentPositions)
    {
        Current.Add(Position);
    }

    TSet<FGridPosition> Wanted;
    Wanted.Reserve(Cells.Num());

    FConnectItBoardLayoutDiff Diff;
    for (const FConnectItBoardLayoutCell& Cell : Cells)
    {
        bool bDuplicate = false;
        Wanted.Add(Cell.Position, &bDuplicate);
        if (!bDuplicate && !Current.Contains(Cell.Position))
        {
            Diff.Added.Add(Cell);
        }
    }
    for (const FGridPosition& Position : CurrentPositions)
    {
        if (!Wanted.Contains(Position))
        {
            Diff.Removed.Add(Position);
        }
    }
    return Diff;
}
