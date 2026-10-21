// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Library/ConnectIt_BoardStateLibrary.h"
#include "Board/Operations/ConnectIt_BoardOperations.h"
#include "Board/Events/ConnectIt_BoardEvents.h"


bool UConnectIt_BoardStateLibrary::GetTileData(
    const FConnectItBoardState& State,
    FGridPosition Position,
    FConnectItTileData& OutData)
{
    if (const FConnectItTileData* Data = State.GetTileData(Position))
    {
        OutData = *Data;
        return true;
    }
    return false;
}

bool UConnectIt_BoardStateLibrary::IsTileOccupied(
    const FConnectItBoardState& State, FGridPosition Position)
{
    return State.IsTileOccupied(Position);
}

bool UConnectIt_BoardStateLibrary::IsTileActive(
    const FConnectItBoardState& State, FGridPosition Position)
{
    return State.IsTileActive(Position);
}

bool UConnectIt_BoardStateLibrary::IsTileValidForPlacement(
    const FConnectItBoardState& State, FGridPosition Position)
{
    return FConnectItBoardOperation_PlacePiece::IsTilePlaceableAt(State, Position);
}

int32 UConnectIt_BoardStateLibrary::GetTileFaction(
    const FConnectItBoardState& State, FGridPosition Position)
{
    const FConnectItTileData* Data = State.GetTileData(Position);
    return Data ? Data->FactionPiece : -1;
}

float UConnectIt_BoardStateLibrary::GetTileMultiplier(
    const FConnectItBoardState& State, FGridPosition Position)
{
    const FConnectItTileData* Data = State.GetTileData(Position);
    return Data ? Data->Multiplier : 1.f;
}

float UConnectIt_BoardStateLibrary::GetFactionScore(
    const FConnectItBoardState& State, int32 FactionSlot)
{
    return State.GetScore(FactionSlot);
}

int32 UConnectIt_BoardStateLibrary::GetNumTiles(const FConnectItBoardState& State)
{
    return State.NumTiles();
}

TArray<FGridPosition> UConnectIt_BoardStateLibrary::GetPositionsForFaction(
    const FConnectItBoardState& State, int32 FactionSlot)
{
    TArray<FGridPosition> Result;
    for (int32 i = 0; i < State.NumTiles(); ++i)
    {
        if (State.GetTileDataAt(i).FactionPiece == FactionSlot)
        {
            Result.Add(State.GetPositionAt(i));
        }
    }
    return Result;
}

TArray<FGridPosition> UConnectIt_BoardStateLibrary::GetPositionsWithMultiplierAbove(
    const FConnectItBoardState& State, float BaseMultiplier)
{
    TArray<FGridPosition> Result;
    for (int32 i = 0; i < State.NumTiles(); ++i)
    {
        if (State.GetTileDataAt(i).Multiplier > BaseMultiplier)
        {
            Result.Add(State.GetPositionAt(i));
        }
    }
    return Result;
}

TArray<FGridPosition> UConnectIt_BoardStateLibrary::GetInactivePositions(
    const FConnectItBoardState& State)
{
    TArray<FGridPosition> Result;
    for (int32 i = 0; i < State.NumTiles(); ++i)
    {
        if (!State.GetTileDataAt(i).bIsActive)
        {
            Result.Add(State.GetPositionAt(i));
        }
    }
    return Result;
}

TArray<FGridPosition> UConnectIt_BoardStateLibrary::GetChangedPositions(
    const FConnectItBoardState& Previous, const FConnectItBoardState& Current)
{
    TArray<FGridPosition> Result;
    for (int32 i = 0; i < Current.NumTiles(); ++i)
    {
        const FGridPosition Position = Current.GetPositionAt(i);
        const FConnectItTileData* PreviousData = Previous.GetTileData(Position);
        const FConnectItTileData& CurrentData = Current.GetTileDataAt(i);

        const bool bChanged = !PreviousData
            || PreviousData->FactionPiece != CurrentData.FactionPiece
            || PreviousData->Multiplier   != CurrentData.Multiplier
            || PreviousData->bIsActive    != CurrentData.bIsActive;

        if (bChanged)
        {
            Result.Add(Position);
        }
    }
    return Result;
}

TArray<FGridPosition> UConnectIt_BoardStateLibrary::GetPlaceablePositions(const FConnectItBoardState& State)
{
    TArray<FGridPosition> Result;
    for (int32 i = 0; i < State.NumTiles(); ++i)
    {
        if (FConnectItBoardOperation_PlacePiece::IsTilePlaceable(State, i))
        {
            Result.Add(State.GetPositionAt(i));
        }
    }
    return Result;
}

EConnectItMoveLegality UConnectIt_BoardStateLibrary::HasAnyLegalMove(
    const FConnectItBoardState& State, FGameplayTag Move, int32 Faction)
{
    return FConnectItBoardOperation::GetLegality(Move, State, Faction);
}

bool UConnectIt_BoardStateLibrary::SimulateOperation(
    const FConnectItBoardState& Board,
    const FConnectItRuleSet& Rules,
    const FInstancedStruct& Operation,
    int32 Faction,
    FConnectItBoardState& ResultBoard,
    float& PointsScored,
    int32& NumScoredEvents,
    FConnectItBoardChangeEvent& Events)
{
    ResultBoard = Board;
    PointsScored = 0.f;
    NumScoredEvents = 0;
    Events = FConnectItBoardChangeEvent();

    // Our own copy of the operation, made by Faction -- what the server does
    // with a request's operation and its requester
    FInstancedStruct OperationStorage = Operation;
    FConnectItBoardOperation* BoardOperation = OperationStorage.GetMutablePtr<FConnectItBoardOperation>();
    if (!BoardOperation) return false;

    BoardOperation->Faction = Faction;
    if (!BoardOperation->CanApply(Board)) return false;

    PointsScored = Rules.CarryOutOperation(*BoardOperation, ResultBoard, &Events);
    Rules.StampWinState(ResultBoard, &Events);

    for (const FInstancedStruct& Entry : Events.Events)
    {
        if (Entry.GetPtr<FConnectItBoardEvent_Scored>())
        {
            NumScoredEvents++;
        }
    }
    return true;
}
