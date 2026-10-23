// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Data/ConnectIt_BoardLayout.h"
#include "ConnectIt_Structs.h"
#include "Misc/DataValidation.h"
#include "Board/ConnectIt_TileBase.h"


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

FConnectItBoardOperation_ChangeLayout UConnectIt_BoardLayout::MakeChangeFrom(const FConnectItBoardState& Board) const
{
    FConnectItBoardOperation_ChangeLayout Change;
    Change.SetDiff(DiffFrom(Board));
    Change.LayoutName = GetFName();
    Change.bHasCamera = bHasCamera;
    Change.CameraTransform = CameraTransform;
    return Change;
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

        // A captured power must be the power of the cell's tile class
        const AConnectIt_TileBase* TileDefaults = Cell.TileClass
            ? Cast<AConnectIt_TileBase>(Cell.TileClass->GetDefaultObject())
            : nullptr;
        if (TileDefaults)
        {
            const FString Problem = AConnectIt_TileBase::CheckTilePower(TileDefaults->TilePowerTag, Cell.TilePower);
            if (!Problem.IsEmpty())
            {
                Context.AddError(FText::FromString(FString::Printf(TEXT(
                    "ConnectIt_BoardLayout: cell (%d,%d), tile class %s: %s. Capture the layout again from the level."),
                    Cell.Position.X, Cell.Position.Y, *Cell.TileClass->GetName(), *Problem)));
                Result = EDataValidationResult::Invalid;
            }
        }
        else if (Cell.TileClass && Cell.TilePower.IsSet())
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_BoardLayout: cell (%d,%d) has power %s but its tile class %s isn't a ConnectIt tile "
                "(AConnectIt_TileBase) -- only those carry powers."),
                Cell.Position.X, Cell.Position.Y, *Cell.TilePower.Name.ToString(), *Cell.TileClass->GetName())));
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
