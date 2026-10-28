// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Library/ConnectIt_BoardEventLibrary.h"
#include "GameEvent/GameEventTaskSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "Interface/GridBoardActor.h"
#include "Piece/GridPieceBase.h"
#include "Piece/GridPieceRegistryBase.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridTileRegistryBase.h"

bool UConnectIt_BoardEventLibrary::GetActivePiecePlacedEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_PiecePlaced& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActivePiecePlacedEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActivePiecesSwappedEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_PiecesSwapped& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActivePiecesSwappedEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActiveBoardShiftedEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_BoardShifted& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActiveBoardShiftedEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActivePieceCapturedEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_PieceCaptured& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActivePieceCapturedEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActivePieceRemovedEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_PieceRemoved& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActivePieceRemovedEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActiveTileMultiplierDestroyedEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_TileMultiplierDestroyed& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActiveTileMultiplierDestroyedEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActiveTileActiveToggledEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_TileActiveToggled& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActiveTileActiveToggledEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActiveGoalsChangedEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_GoalsChanged& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActiveGoalsChangedEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActiveTilesRemovedEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_TilesRemoved& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActiveTilesRemovedEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActiveTilesAddedEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_TilesAdded& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActiveTilesAddedEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActivePhaseChangedEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_PhaseChanged& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActivePhaseChangedEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActiveLayoutChangedEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_LayoutChanged& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActiveLayoutChangedEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActiveScoredEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_Scored& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActiveScoredEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActivePieceMovedEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_PieceMoved& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActivePieceMovedEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActiveMultiplierChangedEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_MultiplierChanged& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActiveMultiplierChangedEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActiveTileLockChangedEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_TileLockChanged& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActiveTileLockChangedEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActiveChainPartnerChangedEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_ChainPartnerChanged& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActiveChainPartnerChangedEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActiveTilePowerTriggeredEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_TilePowerTriggered& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActiveTilePowerTriggeredEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActiveWinCheckpointReachedEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_WinCheckpointReached& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActiveWinCheckpointReachedEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActiveLoseConditionMetEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_LoseConditionMet& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActiveLoseConditionMetEvent"));
}

bool UConnectIt_BoardEventLibrary::GetActiveGameWonEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_GameWon& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActiveGameWonEvent"));
}

const FInstancedStruct* UConnectIt_BoardEventLibrary::GetActiveBoardEvent(const UObject* WorldContextObject)
{
    const UWorld* World = GEngine
        ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)
        : nullptr;
    const UGameEventTaskSubsystem* GameEventSubsystem =
        World ? World->GetSubsystem<UGameEventTaskSubsystem>() : nullptr;
    if (!IsValid(GameEventSubsystem)) return nullptr;

    const FInstancedStruct& Payload = GameEventSubsystem->GetActivePayload();
    return Payload.IsValid() ? &Payload : nullptr;
}

void UConnectIt_BoardEventLibrary::RefreshActorsAt(const UObject* WorldContextObject, const TArray<FGridPosition>& Positions)
{
    const UGridTileRegistryBase* Tiles = UConnectIt_GameUtilityLibrary::GetTileRegistry(WorldContextObject);
    UGridPieceRegistryBase* Pieces = UConnectIt_GameUtilityLibrary::GetPieceRegistry(WorldContextObject);

    for (const FGridPosition& Position : Positions)
    {
        AGridTileBase* Tile = Tiles ? Tiles->GetTileAtPosition(Position) : nullptr;
        if (IsValid(Tile))
        {
            IGridBoardActor::Execute_RefreshFromBoard(Tile);
        }
        AGridPieceBase* Piece = Pieces ? Pieces->GetPieceAtPosition(Position) : nullptr;
        if (IsValid(Piece))
        {
            IGridBoardActor::Execute_RefreshFromBoard(Piece);
        }
    }
}

void UConnectIt_BoardEventLibrary::LogMismatch(const TCHAR* FunctionName, const FInstancedStruct* Active)
{
    if (Active)
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardEventLibrary: %s called, but the board event "
                 "being played is a %s -- use the function that matches the "
                 "event tag being handled"),
            FunctionName, *GetNameSafe(Active->GetScriptStruct()));
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardEventLibrary: %s called, but no board event "
                 "is being played -- only call it while handling a board event "
                 "tag"),
            FunctionName);
    }
}
