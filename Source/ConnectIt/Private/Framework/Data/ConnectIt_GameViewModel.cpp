// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Data/ConnectIt_GameViewModel.h"

#include "Framework/Data/ConnectIt_GameFacade.h"
#include "Tile/GridTileBase.h"
#include "Framework/Data/ConnectIt_PlayerData.h"
#include "Framework/State/ConnectIt_State_Game.h"
#include "Interface/GridFactionInterface.h"
#include "Pooling/ActorPool.h"


UConnectIt_GameViewModel* UConnectIt_GameViewModel::Create(UObject* Outer, UConnectIt_State_Game* InGameState)
{
	UConnectIt_GameViewModel* GameFacade = NewObject<UConnectIt_GameViewModel>();
	GameFacade->GameState = InGameState;
	return GameFacade;
}

void UConnectIt_GameViewModel::PlacePiece(UConnectIt_PlayerData* InPlayerData, AGridTileBase* InGridTile)
{
	if (!InPlayerData || !InGridTile) return;
	InPlayerData->LastTileSelected = InGridTile;
	AActor* Piece = GameState->PlayerPiecePool->GetObjects(1)[0];
	if (!Piece) return;
	if (Piece->Implements<IGridFactionInterface>)
	{
		IGridFactionInterface::Execute_SetFactionId(Piece, InPlayerData->PlayerId);
	}
	InGridTile->PlacePiece(Piece);
	InPlayerData->LastPiecePlaced = Piece;
}

void UConnectIt_GameViewModel::SetPlayerScore(const int32 PlayerId, const int32 InScore)
{
	if (!GameState) return;
	UConnectIt_PlayerData* PlayerData = GameState->GameFacade->GetPlayerDataById(PlayerId);
	if (!PlayerData) return;
	PlayerData->AddToScore(InScore);
}
