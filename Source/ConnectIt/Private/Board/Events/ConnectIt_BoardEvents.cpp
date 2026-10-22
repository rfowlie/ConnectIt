// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Events/ConnectIt_BoardEvents.h"
#include "ConnectIt_GameplayTags.h"


FGameplayTag FConnectItBoardEvent_BoardSeeded::GetEventTag() const
{
    return ConnectIt_Event_BoardSeeded;
}

FGameplayTag FConnectItBoardEvent_PiecePlaced::GetEventTag() const
{
    return ConnectIt_Event_PiecePlaced;
}

FGameplayTag FConnectItBoardEvent_PiecesSwapped::GetEventTag() const
{
    return ConnectIt_Event_PiecesSwapped;
}

FGameplayTag FConnectItBoardEvent_BoardShifted::GetEventTag() const
{
    return ConnectIt_Event_BoardShifted;
}

FGameplayTag FConnectItBoardEvent_PieceCaptured::GetEventTag() const
{
    return ConnectIt_Event_PieceCaptured;
}

FGameplayTag FConnectItBoardEvent_PieceRemoved::GetEventTag() const
{
    return ConnectIt_Event_PieceRemoved;
}

FGameplayTag FConnectItBoardEvent_TileMultiplierDestroyed::GetEventTag() const
{
    return ConnectIt_Event_TileMultiplierDestroyed;
}

FGameplayTag FConnectItBoardEvent_TileActiveToggled::GetEventTag() const
{
    return ConnectIt_Event_TileActiveToggled;
}

FGameplayTag FConnectItBoardEvent_GoalsChanged::GetEventTag() const
{
    return ConnectIt_Event_GoalsChanged;
}

FGameplayTag FConnectItBoardEvent_TilesRemoved::GetEventTag() const
{
    return ConnectIt_Event_TilesRemoved;
}

FGameplayTag FConnectItBoardEvent_TilesAdded::GetEventTag() const
{
    return ConnectIt_Event_TilesAdded;
}

FGameplayTag FConnectItBoardEvent_LayoutChanged::GetEventTag() const
{
    return ConnectIt_Event_LayoutChanged;
}

FGameplayTag FConnectItBoardEvent_Scored::GetEventTag() const
{
    return ConnectIt_Event_Scored;
}

FGameplayTag FConnectItBoardEvent_WinCheckpointReached::GetEventTag() const
{
    return ConnectIt_Event_WinCheckpointReached;
}

FGameplayTag FConnectItBoardEvent_LoseConditionMet::GetEventTag() const
{
    return ConnectIt_Event_LoseConditionMet;
}

FGameplayTag FConnectItBoardEvent_GameWon::GetEventTag() const
{
    return ConnectIt_Event_PlayerWin;
}
