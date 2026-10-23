// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Data/ConnectIt_BoardLayout.h"
#include "ConnectIt_Structs.h"
#include "Misc/DataValidation.h"
#include "Board/Powers/ConnectIt_TileCatalog.h"
#include "ConnectIt_GameplayTags.h"


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

    const UConnectIt_TileCatalog* Catalog = UConnectIt_TileCatalog::GetProjectDefault();
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

        // Its power tag must be one the catalog defines (the project default:
        // a layout isn't tied to one level)
        if (Catalog && Cell.TileData.TilePowerTag.IsValid() && !Catalog->Defines(Cell.TileData.TilePowerTag))
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_BoardLayout: cell (%d,%d) names power %s, which tile catalog '%s' doesn't define."),
                Cell.Position.X, Cell.Position.Y, *Cell.TileData.TilePowerTag.ToString(), *Catalog->GetName())));
            Result = EDataValidationResult::Invalid;
        }

        // A chain partner: on a Chained cell, naming another cell of this layout
        if (Cell.TileData.bHasChainPartner)
        {
            const FConnectItBoardLayoutCell* Partner = FindCell(Cell.TileData.ChainPartner);
            if (Cell.TileData.TilePowerTag != ConnectIt_TilePower_Chained)
            {
                Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                    "ConnectIt_BoardLayout: cell (%d,%d) has a chain partner but isn't Chained -- it is ignored."),
                    Cell.Position.X, Cell.Position.Y)));
            }
            else if (!Partner || Partner == &Cell)
            {
                Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                    "ConnectIt_BoardLayout: cell (%d,%d)'s chain partner (%d,%d) isn't another cell of this layout -- it "
                    "will have no partner."),
                    Cell.Position.X, Cell.Position.Y, Cell.TileData.ChainPartner.X, Cell.TileData.ChainPartner.Y)));
            }
            else if (Partner->TileData.TilePowerTag == ConnectIt_TilePower_Chained)
            {
                Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                    "ConnectIt_BoardLayout: cell (%d,%d)'s chain partner is Chained too -- a chain of chains isn't "
                    "tested yet."), Cell.Position.X, Cell.Position.Y)));
            }
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
