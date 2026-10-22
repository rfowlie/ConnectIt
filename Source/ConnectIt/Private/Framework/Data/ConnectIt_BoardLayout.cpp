// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Data/ConnectIt_BoardLayout.h"
#include "ConnectIt_Structs.h"
#include "Misc/DataValidation.h"


const FConnectItBoardLayoutCell* UConnectIt_BoardLayout::FindCell(const FGridPosition& Position) const
{
    return Cells.FindByPredicate([&Position](const FConnectItBoardLayoutCell& Cell)
    {
        return Cell.Position == Position;
    });
}

FConnectItBoardLayoutDiff UConnectIt_BoardLayout::DiffFrom(const FConnectItBoardState& Board) const
{
    return FConnectItBoardLayoutDiff::Between(Board.TilePositions, Cells);
}

FConnectItBoardLayoutDiff UConnectIt_BoardLayout::DiffFrom(const UConnectIt_BoardLayout& Other) const
{
    TArray<FGridPosition> OtherPositions;
    OtherPositions.Reserve(Other.Cells.Num());
    for (const FConnectItBoardLayoutCell& Cell : Other.Cells)
    {
        OtherPositions.Add(Cell.Position);
    }
    return FConnectItBoardLayoutDiff::Between(OtherPositions, Cells);
}

#if WITH_EDITOR
EDataValidationResult UConnectIt_BoardLayout::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (Cells.IsEmpty())
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_BoardLayout: No Cells -- changing to this layout removes every tile.")));
    }

    // Every match today has two factions (AConnectIt_GameMode::NumFactions)
    constexpr int32 MatchFactions = 2;

    TSet<FGridPosition> Seen;
    for (int32 Index = 0; Index < Cells.Num(); Index++)
    {
        const FConnectItBoardLayoutCell& Cell = Cells[Index];

        bool bDuplicate = false;
        Seen.Add(Cell.Position, &bDuplicate);
        if (bDuplicate)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_BoardLayout: cell %d repeats position (%d,%d) -- one tile per position."),
                Index, Cell.Position.X, Cell.Position.Y)));
            Result = EDataValidationResult::Invalid;
        }

        if (Cell.TileData.StartingFactionPiece >= MatchFactions)
        {
            Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_BoardLayout: cell (%d,%d) starts with a piece for faction %d, but matches have %d "
                "factions -- the tile would start empty."),
                Cell.Position.X, Cell.Position.Y, Cell.TileData.StartingFactionPiece, MatchFactions)));
        }
    }
    return Result;
}
#endif
