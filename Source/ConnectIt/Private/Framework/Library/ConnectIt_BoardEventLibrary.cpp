// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Library/ConnectIt_BoardEventLibrary.h"
#include "GameEvent/GameEventTaskSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

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

bool UConnectIt_BoardEventLibrary::GetActiveScoredEvent(
    const UObject* WorldContextObject, FConnectItBoardEvent_Scored& OutEvent)
{
    return TryGetActiveBoardEvent(WorldContextObject, OutEvent, TEXT("GetActiveScoredEvent"));
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
