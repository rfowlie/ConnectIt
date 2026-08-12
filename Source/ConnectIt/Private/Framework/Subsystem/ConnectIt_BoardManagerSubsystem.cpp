// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Subsystem/ConnectIt_BoardManagerSubsystem.h"


void UConnectIt_BoardManagerSubsystem::RegisterBoardManager(AConnectIt_BoardManager* InBoardManager)
{
	if (!IsValid(InBoardManager)) return;

	UnbindFromBoardManager();

	CachedBoardManager = InBoardManager;
	BindToBoardManager(InBoardManager);

	OnBoardManagerReady.Broadcast(InBoardManager);
}

void UConnectIt_BoardManagerSubsystem::BindToBoardManager(AConnectIt_BoardManager* InBoardManager)
{
	InBoardManager->OnPiecePlaced.AddDynamic(this, &UConnectIt_BoardManagerSubsystem::HandlePiecePlaced);
	InBoardManager->OnLineScored.AddDynamic(this, &UConnectIt_BoardManagerSubsystem::HandleLineScored);
	InBoardManager->OnPlayerWin.AddDynamic(this, &UConnectIt_BoardManagerSubsystem::HandlePlayerWin);
	InBoardManager->OnShiftApplied.AddDynamic(this, &UConnectIt_BoardManagerSubsystem::HandleShiftApplied);
}

void UConnectIt_BoardManagerSubsystem::UnbindFromBoardManager()
{
	if (!IsValid(CachedBoardManager)) return;

	CachedBoardManager->OnPiecePlaced.RemoveDynamic(this, &UConnectIt_BoardManagerSubsystem::HandlePiecePlaced);
	CachedBoardManager->OnLineScored.RemoveDynamic(this, &UConnectIt_BoardManagerSubsystem::HandleLineScored);
	CachedBoardManager->OnPlayerWin.RemoveDynamic(this, &UConnectIt_BoardManagerSubsystem::HandlePlayerWin);
	CachedBoardManager->OnShiftApplied.RemoveDynamic(this, &UConnectIt_BoardManagerSubsystem::HandleShiftApplied);
}

void UConnectIt_BoardManagerSubsystem::HandlePiecePlaced(FGridPosition Position)
{
	OnPiecePlaced.Broadcast(Position);
}

void UConnectIt_BoardManagerSubsystem::HandleLineScored(int32 FactionSlot, float PointsScored)
{
	OnLineScored.Broadcast(FactionSlot, PointsScored);
}

void UConnectIt_BoardManagerSubsystem::HandlePlayerWin(int32 WinningFactionSlot)
{
	OnPlayerWin.Broadcast(WinningFactionSlot);
}

void UConnectIt_BoardManagerSubsystem::HandleShiftApplied(const FShiftOperation& Operation, const FShiftResult& Result)
{
	OnShiftApplied.Broadcast(Operation, Result);
}
