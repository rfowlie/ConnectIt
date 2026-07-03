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
	UConnectIt_State_Game* Obj = NewObject<UConnectIt_State_Game>(Outer);
	
	Obj->GameTurnTracker = NewObject<UGameTurnTracker>(Outer);
	
	Obj->GameFacade = UConnectIt_GameFacade::Create(Outer, Obj);
	Obj->GameViewModel = UConnectIt_GameViewModel::Create(Outer, Obj);
	// PlayerPiecePool = UActorPool::Create(AGridPieceBase::StaticClass(), 64);
	
	Obj->StatePlayerTurn = UConnectIt_State_SelectTile::Create(Outer, Obj->GameFacade, Obj->GameViewModel);
	Obj->StatePlayerTurn->OnStateComplete.AddDynamic(Obj, &ThisClass::OnTileSelected);

	Obj->StatePlacePiece = UConnectIt_State_PlacePiece::Create(Outer, Obj->GameFacade, Obj->GameViewModel);
	Obj->StatePlacePiece->OnStateComplete.AddDynamic(Obj, &ThisClass::OnPiecePlaced);

	Obj->StateUpdateBoard = UConnectIt_State_UpdateGameBoard::Create(Outer, Obj->GameFacade, Obj->GameViewModel);
	Obj->StateUpdateBoard->OnStateComplete.AddDynamic(Obj, &ThisClass::OnBoardUpdated);

	return Obj;
}

void UConnectIt_State_Game::Initialize(UObject* Outer)
{
	GameTurnTracker = NewObject<UGameTurnTracker>(Outer);
	
	GameFacade = UConnectIt_GameFacade::Create(Outer, this);
	GameViewModel = UConnectIt_GameViewModel::Create(Outer, this);
	// PlayerPiecePool = UActorPool::Create(AGridPieceBase::StaticClass(), 64);
	
	StatePlayerTurn = UConnectIt_State_SelectTile::Create(Outer, GameFacade, GameViewModel);
	StatePlayerTurn->OnStateComplete.AddDynamic(this, &ThisClass::OnTileSelected);
	StatePlacePiece = UConnectIt_State_PlacePiece::Create(Outer, GameFacade, GameViewModel);
	StatePlacePiece->OnStateComplete.AddDynamic(this, &ThisClass::OnPiecePlaced);
	StateUpdateBoard = UConnectIt_State_UpdateGameBoard::Create(Outer, GameFacade, GameViewModel);
	StateUpdateBoard->OnStateComplete.AddDynamic(this, &ThisClass::OnBoardUpdated);
}

void UConnectIt_State_Game::BroadCastGameState(UConnectIt_State_Base* GameState) const
{
	// TODO: why is this not working? Because not Tag set?
	return;
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
