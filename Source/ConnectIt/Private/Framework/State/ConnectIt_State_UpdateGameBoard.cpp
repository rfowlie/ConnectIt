// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/State/ConnectIt_State_UpdateGameBoard.h"
#include "ConnectIt_GameplayTags.h"

UConnectIt_State_UpdateGameBoard* UConnectIt_State_UpdateGameBoard::Create(
	UObject* Outer,	UConnectIt_GameFacade* InGameFacade, UConnectIt_GameViewModel* InViewModel)
{
	UConnectIt_State_UpdateGameBoard* Obj = NewObject<UConnectIt_State_UpdateGameBoard>();
	Obj->GameStateTag = ConnectIt_Game_UpdateBoard;
	Obj->GameFacade = InGameFacade;
	Obj->GameViewModel = InViewModel;
	return Obj;
}