// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/State/ConnectIt_State_Game.h"
#include "Framework/Data/ConnectIt_GameFacade.h"
#include "Framework/Data/ConnectIt_GameViewModel.h"
#include "Framework/State/ConnectIt_State_PlacePiece.h"
#include "Framework/State/ConnectIt_State_UpdateGameBoard.h"
#include "Framework/State/ConnectIt_State_SelectTile.h"
#include "GameTurn/GameTurnTracker.h"
#include "Tile/GridTileBase.h"


UConnectIt_State_Game::UConnectIt_State_Game()
{
}

UConnectIt_State_Game* UConnectIt_State_Game::Create(UObject* Outer)
{
	if (!Outer) { return nullptr; }
	UConnectIt_State_Game* Obj = NewObject<UConnectIt_State_Game>(Outer);
	
	Obj->GameTurnTracker = NewObject<UGameTurnTracker>(Outer);
	
	Obj->GameFacade = UConnectIt_GameFacade::Create(Outer, Obj);
	Obj->GameViewModel = UConnectIt_GameViewModel::Create(Outer, Obj, Obj->GameFacade);
	// PlayerPiecePool = UActorPool::Create(AGridPieceBase::StaticClass(), 64);
	
	Obj->ConstructStateSelectTile(Outer);
	Obj->ConstructStatePlacePiece(Outer);
	Obj->ConstructStateUpdateGameBoard(Outer);
	
	return Obj;
}

void UConnectIt_State_Game::Initialize(UObject* Outer)
{
	GameTurnTracker = NewObject<UGameTurnTracker>(Outer);
	
	GameFacade = UConnectIt_GameFacade::Create(Outer, this);
	GameViewModel = UConnectIt_GameViewModel::Create(Outer, this, GameFacade);
	// PlayerPiecePool = UActorPool::Create(AGridPieceBase::StaticClass(), 64);
	
	ConstructStateSelectTile(Outer);
	ConstructStatePlacePiece(Outer);
	ConstructStateUpdateGameBoard(Outer);
}

void UConnectIt_State_Game::ConstructStateSelectTile_Implementation(UObject* Outer)
{
	StatePlayerTurn = UConnectIt_State_SelectTile::Create(Outer, GameFacade, GameViewModel);
	StatePlayerTurn->OnStateComplete.AddDynamic(this, &ThisClass::OnTileSelected);
}

void UConnectIt_State_Game::ConstructStatePlacePiece_Implementation(UObject* Outer)
{
	StatePlacePiece = UConnectIt_State_PlacePiece::Create(Outer, GameFacade, GameViewModel);
	StatePlacePiece->OnStateComplete.AddDynamic(this, &ThisClass::OnPiecePlaced);
}

void UConnectIt_State_Game::ConstructStateUpdateGameBoard_Implementation(UObject* Outer)
{
	StateUpdateBoard = UConnectIt_State_UpdateGameBoard::Create(Outer, GameFacade, GameViewModel);
	StateUpdateBoard->OnStateComplete.AddDynamic(this, &ThisClass::OnBoardUpdated);
}

void UConnectIt_State_Game::BroadCastGameState(UConnectIt_State_Base* GameState) const
{
	// TODO: why is this not working? Because not Tag set?
	// return;
	if (!GameState) { return; }
	if (OnGameStateChanged.IsBound()) { OnGameStateChanged.Broadcast(GameState->GetGameStateTag()); }
}

void UConnectIt_State_Game::OnTileSelected_Implementation()
{
	if (!StatePlayerTurn->GetSelectedTile()) return;
	StatePlayerTurn->Exit_Implementation();
	StatePlacePiece->SetSelectedTile(StatePlayerTurn->GetSelectedTile());
	BroadCastGameState(StatePlacePiece);
	StatePlacePiece->Enter_Implementation();	
}

void UConnectIt_State_Game::OnPiecePlaced_Implementation()
{
	StatePlacePiece->Exit_Implementation();
	BroadCastGameState(StateUpdateBoard);
	StateUpdateBoard->Enter_Implementation();
}


void UConnectIt_State_Game::OnBoardUpdated_Implementation()
{
	StateUpdateBoard->Exit_Implementation();
	
	if (GameFacade->IsGameBoardFull() || GameFacade->CheckPlayerWon(GameFacade->GetCurrentPlayerData()))
	{
		// calling complete on this state so game mode can push next state
		if (OnStateComplete.IsBound()) { OnStateComplete.Broadcast(); }
	}
	else
	{
		// UpdateGameInfo();
		StartNextPlayerTurn();
	}
}

void UConnectIt_State_Game::StartNextPlayerTurn_Implementation()
{
	GameTurnTracker->Increment();
	BroadCastGameState(StatePlayerTurn);
	StatePlayerTurn->Enter_Implementation();
}
