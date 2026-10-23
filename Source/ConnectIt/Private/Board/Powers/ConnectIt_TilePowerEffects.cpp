// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Powers/ConnectIt_TilePowerEffects.h"
#include "Board/Operations/ConnectIt_BoardOperations.h"
#include "ConnectIt_Structs.h"


void FConnectItTilePowerEffect_Chained::MakeOperations(
    const FConnectItTilePowerContext& Context, TArray<FInstancedStruct>& OutOperations) const
{
    const FConnectItTileData* Tile = Context.Board.GetTileData(Context.Position);
    if (!Tile) return;

    const int32 PartnerIndex = Context.Board.FindTileIndexById(Tile->ChainPartnerId);
    const bool bPartnerHasPiece = Context.Board.TileDataArray.IsValidIndex(PartnerIndex)
        && Context.Board.TileDataArray[PartnerIndex].bIsOccupied;

    FConnectItBoardOperation_SetTileLocked SetLocked;
    SetLocked.Position = Context.Position;

    if (bPartnerHasPiece && Tile->bIsLocked)
    {
        SetLocked.bLocked = false;
        OutOperations.Add(FInstancedStruct::Make(SetLocked));
    }
    else if (!bPartnerHasPiece && !Tile->bIsLocked)
    {
        // The piece goes first: a locked tile's piece can't be removed
        if (Tile->bIsOccupied)
        {
            FConnectItBoardOperation_RemovePiece Remove;
            Remove.Position = Context.Position;
            OutOperations.Add(FInstancedStruct::Make(Remove));
        }
        SetLocked.bLocked = true;
        OutOperations.Add(FInstancedStruct::Make(SetLocked));
    }
}
