// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/State/ConnectIt_State_Game.h"
#include "Framework/Data/ConnectIt_GameFacade.h"
#include "Framework/Data/ConnectIt_GameViewModel.h"
#include "Framework/Data/ConnectIt_PlayerData.h"
#include "Framework/State/ConnectIt_State_PlacePiece.h"
#include "Framework/State/ConnectIt_State_UpdateGameBoard.h"
#include "Framework/State/ConnectIt_State_SelectTile.h"
#include "GameTurn/GameTurnTracker.h"
#include "Interface/GridPieceHandler.h"
#include "Piece/GridPieceBase.h"
#include "Pooling/ActorPool.h"
#include "Tile/GridTileBase.h"


UConnectIt_State_Game::UConnectIt_State_Game()
{
	GameTurnTracker = NewObject<UGameTurnTracker>();
	
	GameFacade = UConnectIt_GameFacade::Create(this, this);
	GameViewModel = UConnectIt_GameViewModel::Create(this, this);
	// PlayerPiecePool = UActorPool::Create(AGridPieceBase::StaticClass(), 64);
	
	StatePlayerTurn = UConnectIt_State_SelectTile::Create(this, GameFacade, GameViewModel);
	StatePlayerTurn->OnStateComplete.AddDynamic(this, &ThisClass::OnTileSelected);
	StatePlacePiece = UConnectIt_State_PlacePiece::Create(this, GameFacade, GameViewModel);
	StatePlacePiece->OnStateComplete.AddDynamic(this, &ThisClass::OnPiecePlaced);
	StateUpdateBoard = UConnectIt_State_UpdateGameBoard::Create(this, GameFacade, GameViewModel);
	StateUpdateBoard->OnStateComplete.AddDynamic(this, &ThisClass::OnBoardUpdated);
	
}

void UConnectIt_State_Game::SetWorldContext_Implementation(UWorld* InWorldContext)
{
	if (!InWorldContext) { return; }
	WorldContext = InWorldContext;
}

void UConnectIt_State_Game::OnTileSelected_Implementation()
{
	if (!StatePlayerTurn->GetSelectedTile()) return;
	StatePlayerTurn->Exit_Implementation();
	StatePlacePiece->SetSelectedTile(StatePlayerTurn->GetSelectedTile());
	StatePlacePiece->Enter_Implementation();	
}

void UConnectIt_State_Game::OnPiecePlaced_Implementation()
{
	StatePlacePiece->Exit_Implementation();
	StateUpdateBoard->Enter_Implementation();
}


void UConnectIt_State_Game::OnBoardUpdated_Implementation()
{
	StateUpdateBoard->Exit_Implementation();
	
	if (IsGameBoardFull() || CheckPlayerWon(GameFacade->GetCurrentPlayerData()))
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


bool UConnectIt_State_Game::IsGameBoardFull_Implementation()
{
	for (const auto Tile : AllGridTiles)
	{
		if (!IGridPieceHandler::Execute_HasPiece(Tile))
		{
			return false;
		}
	}

	return true;
}

bool UConnectIt_State_Game::CheckPlayerWon_Implementation(const UConnectIt_PlayerData* InPlayerData)
{
	if (!InPlayerData) return false;
	return InPlayerData->Score >= 100;
}


void UConnectIt_State_Game::StartNextPlayerTurn_Implementation()
{
	GameTurnTracker->Increment();
	StatePlayerTurn->Enter_Implementation();
}
