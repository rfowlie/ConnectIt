// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/State/ConnectIt_State_UpdateGameBoard.h"

UConnectIt_State_UpdateGameBoard* UConnectIt_State_UpdateGameBoard::Create(
	UObject* Outer,	UConnectIt_GameFacade* InGameFacade, UConnectIt_GameViewModel* InViewModel)
{
	UConnectIt_State_UpdateGameBoard* State = NewObject<UConnectIt_State_UpdateGameBoard>();
	State->GameFacade = InGameFacade;
	State->GameViewModel = InViewModel;
	return State;
}
