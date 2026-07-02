// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/State/ConnectIt_State_PlacePiece.h"

UConnectIt_State_PlacePiece* UConnectIt_State_PlacePiece::Create(
	UObject* Outer, UConnectIt_GameFacade* InGameFacade, UConnectIt_GameViewModel* InViewModel)
{
	UConnectIt_State_PlacePiece* State = NewObject<UConnectIt_State_PlacePiece>();
	State->GameFacade = InGameFacade;
	State->GameViewModel = InViewModel;
	return State;
}
