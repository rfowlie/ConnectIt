// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Powers/ConnectIt_TilePowerEffects.h"
#include "Board/Operations/ConnectIt_BoardOperations.h"
#include "ConnectIt_Structs.h"
#include "Board/Rules/ConnectIt_RuleSet.h"


namespace
{
    // A Move Piece From -> To, if the rules would allow it now (no rules: its own check only)
    bool CanMove(const FConnectItTilePowerContext& Context, const FGridPosition& From, const FGridPosition& To,
        FConnectItBoardOperation_MovePiece& OutMove)
    {
        OutMove.Faction = Context.Faction;
        OutMove.From = From;
        OutMove.To = To;
        return Context.Rules ? Context.Rules->CanCarryOut(OutMove, Context.Board) : OutMove.CanApply(Context.Board);
    }

    // One of Candidates, picked with the board's seeded stream
    void AddRandomMove(const FConnectItTilePowerContext& Context,
        const TArray<FConnectItBoardOperation_MovePiece, TInlineAllocator<8>>& Candidates, TArray<FInstancedStruct>& OutOperations)
    {
        if (Candidates.IsEmpty()) return;
        const int32 Pick = Candidates.Num() == 1 ? 0 : Context.Random.RandRange(0, Candidates.Num() - 1);
        OutOperations.Add(FInstancedStruct::Make(Candidates[Pick]));
    }

    const FIntPoint OrthogonalDirections[] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    const FIntPoint AllDirections[] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 }, { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 } };

    FGridPosition Step(const FGridPosition& From, const FIntPoint& Direction, int32 Steps = 1)
    {
        return FGridPosition(From.X + Direction.X * Steps, From.Y + Direction.Y * Steps);
    }
}

void FConnectItTilePowerEffect_Push::MakeOperations(
    const FConnectItTilePowerContext& Context, TArray<FInstancedStruct>& OutOperations) const
{
    TArray<FConnectItBoardOperation_MovePiece, TInlineAllocator<8>> Candidates;
    for (const FIntPoint& Direction : OrthogonalDirections)
    {
        FConnectItBoardOperation_MovePiece Move;
        if (CanMove(Context, Step(Context.Position, Direction), Step(Context.Position, Direction, 2), Move))
        {
            Candidates.Add(Move);
        }
    }
    AddRandomMove(Context, Candidates, OutOperations);
}

void FConnectItTilePowerEffect_Vacuum::MakeOperations(
    const FConnectItTilePowerContext& Context, TArray<FInstancedStruct>& OutOperations) const
{
    const FConnectItTileData* Own = Context.Board.GetTileData(Context.Position);
    if (!Own || Own->bIsOccupied) return;

    TArray<FConnectItBoardOperation_MovePiece, TInlineAllocator<8>> Candidates;
    for (const FIntPoint& Direction : AllDirections)
    {
        // The nearest piece this way, over empty tiles
        for (int32 Steps = 1;; Steps++)
        {
            const FGridPosition Look = Step(Context.Position, Direction, Steps);
            const FConnectItTileData* Tile = Context.Board.GetTileData(Look);
            if (!Tile || !Tile->bIsActive) break;
            if (!Tile->bIsOccupied) continue;

            FConnectItBoardOperation_MovePiece Move;
            if (CanMove(Context, Look, Context.Position, Move))
            {
                Candidates.Add(Move);
            }
            break;
        }
    }
    AddRandomMove(Context, Candidates, OutOperations);
}

void FConnectItTilePowerEffect_Repel::MakeOperations(
    const FConnectItTilePowerContext& Context, TArray<FInstancedStruct>& OutOperations) const
{
    if (!Context.Board.IsTileOccupied(Context.Position)) return;

    TArray<FConnectItBoardOperation_MovePiece, TInlineAllocator<8>> Candidates;
    for (const FIntPoint& Direction : AllDirections)
    {
        FConnectItBoardOperation_MovePiece Move;
        if (CanMove(Context, Context.Position, Step(Context.Position, Direction), Move))
        {
            Candidates.Add(Move);
        }
    }
    AddRandomMove(Context, Candidates, OutOperations);
}

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
