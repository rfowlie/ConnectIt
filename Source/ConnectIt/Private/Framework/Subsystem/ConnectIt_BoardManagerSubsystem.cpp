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
	InBoardManager->OnShiftApplied.AddDynamic(this, &UConnectIt_BoardManagerSubsystem::HandleShiftApplied);
}

void UConnectIt_BoardManagerSubsystem::UnbindFromBoardManager()
{
	if (!IsValid(CachedBoardManager)) return;

	CachedBoardManager->OnShiftApplied.RemoveDynamic(this, &UConnectIt_BoardManagerSubsystem::HandleShiftApplied);
}

void UConnectIt_BoardManagerSubsystem::HandleShiftApplied(const FShiftOperation& Operation, const FShiftResult& Result)
{
	OnShiftApplied.Broadcast(Operation, Result);
}
