// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "ConnectIt_BoardEventLibrary.generated.h"

// Reads the board event being played right now.
//
// Board events (ConnectIt_BoardEvents.h) are played one at a time through
// UGameEventTaskSubsystem: each is queued with its gameplay tag
// (ConnectIt.Event.*) and the event itself as the queue entry's payload. A
// listener bound to one of those tags gets its data here: one function per
// kind of event, each returning that event's own struct. Call the one
// matching the tag you are handling.
//
// Each returns false (the node's False pin) and logs an error if that kind of
// event is not the one being played -- e.g. the wrong function for the tag,
// or called outside an event. One function per type, rather than a single
// untyped one, so a mismatch is loud instead of a silently empty struct.
//
// A library rather than functions on UConnectIt_BoardStateComponent: the
// data comes from the event queue, not from the board state, so nothing here
// needs that component. A new kind of event with data gets a function here.
UCLASS()
class CONNECTIT_API UConnectIt_BoardEventLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    // For listeners of ConnectIt.Event.PiecePlaced
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board Events",
        meta = (WorldContext = "WorldContextObject", ExpandBoolAsExecs = "ReturnValue"))
    static bool GetActivePiecePlacedEvent(const UObject* WorldContextObject, FConnectItBoardEvent_PiecePlaced& OutEvent);

    // For listeners of ConnectIt.Event.PiecesSwapped
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board Events",
        meta = (WorldContext = "WorldContextObject", ExpandBoolAsExecs = "ReturnValue"))
    static bool GetActivePiecesSwappedEvent(const UObject* WorldContextObject, FConnectItBoardEvent_PiecesSwapped& OutEvent);

    // For listeners of ConnectIt.Event.BoardShifted
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board Events",
        meta = (WorldContext = "WorldContextObject", ExpandBoolAsExecs = "ReturnValue"))
    static bool GetActiveBoardShiftedEvent(const UObject* WorldContextObject, FConnectItBoardEvent_BoardShifted& OutEvent);

    // For listeners of ConnectIt.Event.PieceCaptured
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board Events",
        meta = (WorldContext = "WorldContextObject", ExpandBoolAsExecs = "ReturnValue"))
    static bool GetActivePieceCapturedEvent(const UObject* WorldContextObject, FConnectItBoardEvent_PieceCaptured& OutEvent);

    // For listeners of ConnectIt.Event.PieceRemoved
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board Events",
        meta = (WorldContext = "WorldContextObject", ExpandBoolAsExecs = "ReturnValue"))
    static bool GetActivePieceRemovedEvent(const UObject* WorldContextObject, FConnectItBoardEvent_PieceRemoved& OutEvent);

    // For listeners of ConnectIt.Event.TileMultiplierDestroyed
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board Events",
        meta = (WorldContext = "WorldContextObject", ExpandBoolAsExecs = "ReturnValue"))
    static bool GetActiveTileMultiplierDestroyedEvent(const UObject* WorldContextObject, FConnectItBoardEvent_TileMultiplierDestroyed& OutEvent);

    // For listeners of ConnectIt.Event.TileActiveToggled
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board Events",
        meta = (WorldContext = "WorldContextObject", ExpandBoolAsExecs = "ReturnValue"))
    static bool GetActiveTileActiveToggledEvent(const UObject* WorldContextObject, FConnectItBoardEvent_TileActiveToggled& OutEvent);

    // For listeners of ConnectIt.Event.Scored
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board Events",
        meta = (WorldContext = "WorldContextObject", ExpandBoolAsExecs = "ReturnValue"))
    static bool GetActiveScoredEvent(const UObject* WorldContextObject, FConnectItBoardEvent_Scored& OutEvent);

    // For listeners of ConnectIt.Event.PlayerWin
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board Events",
        meta = (WorldContext = "WorldContextObject", ExpandBoolAsExecs = "ReturnValue"))
    static bool GetActiveGameWonEvent(const UObject* WorldContextObject, FConnectItBoardEvent_GameWon& OutEvent);

    // C++ form: the board event being played if it is a TEvent, else null
    // (no error logged). The pointer is only good for the duration of the
    // listener call.
    template<typename TEvent>
    static const TEvent* GetActiveBoardEventAs(const UObject* WorldContextObject)
    {
        const FInstancedStruct* Active = GetActiveBoardEvent(WorldContextObject);
        return Active ? Active->GetPtr<TEvent>() : nullptr;
    }

private:

    // The game event queue's active payload, or null if there is none
    static const FInstancedStruct* GetActiveBoardEvent(const UObject* WorldContextObject);

    // Shared body of the GetActive...Event functions
    template<typename TEvent>
    static bool TryGetActiveBoardEvent(
        const UObject* WorldContextObject, TEvent& OutEvent, const TCHAR* FunctionName)
    {
        const FInstancedStruct* Active = GetActiveBoardEvent(WorldContextObject);
        if (const TEvent* Event = Active ? Active->GetPtr<TEvent>() : nullptr)
        {
            OutEvent = *Event;
            return true;
        }

        LogMismatch(FunctionName, Active);
        return false;
    }

    static void LogMismatch(const TCHAR* FunctionName, const FInstancedStruct* Active);
};
