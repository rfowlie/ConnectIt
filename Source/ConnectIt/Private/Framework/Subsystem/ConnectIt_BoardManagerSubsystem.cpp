// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Subsystem/ConnectIt_BoardManagerSubsystem.h"


void UConnectIt_BoardManagerSubsystem::RegisterBoardManager(AConnectIt_BoardManager* InBoardManager)
{
	if (!IsValid(InBoardManager)) return;

	CachedBoardManager = InBoardManager;

	OnBoardManagerReady.Broadcast(InBoardManager);
}
