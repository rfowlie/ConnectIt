// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Data/ConnectIt_GameFacade.h"

#include "GridMechanics_GridLibrary.h"
#include "GridMechanics_ShapeLibrary.h"
#include "Framework/Data/ConnectIt_PlayerData.h"
#include "Framework/State/ConnectIt_State_Game.h"
#include "GameTurn/GameTurnTracker.h"
#include "Interface/GridFactionInterface.h"
#include "Interface/GridLevelInterface.h"
#include "Interface/GridPieceHandler.h"
#include "Interface/GridTileHandler.h"
#include "Library/ConnectIt_GridRulesLibrary.h"
#include "Tile/GridTileBase.h"


UConnectIt_GameFacade* UConnectIt_GameFacade::Create(UObject* Outer, UConnectIt_State_Game* InGameState)
{
	UConnectIt_GameFacade* GameFacade = NewObject<UConnectIt_GameFacade>();
	GameFacade->GameState = InGameState;
	return GameFacade;
}

TArray<AGridTileBase*> UConnectIt_GameFacade::GetGridTiles() const
{
	if (!GameState)
	{
		TArray<AGridTileBase*> OutGridTiles;
		return OutGridTiles;
	}
	return GameState->AllGridTiles;
}

TArray<AGridTileBase*> UConnectIt_GameFacade::GetEmptyGridTiles() const
{
	TArray<AGridTileBase*> OutGridTiles;
	if (!GameState) { return OutGridTiles; }
	for (const auto GridTile : GetGridTiles())
	{
		if (IsTileEmpty(GridTile))
		{
			OutGridTiles.Add(GridTile);
		}
	}
	
	return OutGridTiles;
}

TArray<AGridTileBase*> UConnectIt_GameFacade::GetGridTilesWithPlayerPieces(const int32 PlayerID) const
{
	TArray<AGridTileBase*> OutGridTiles;
	if (!GameState) { return OutGridTiles; }
	for (const auto GridTile : GetGridTiles())
	{
		if (GridTile->Implements<IGridFactionInterface>())
		{
			if (PlayerID == IGridFactionInterface::Execute_GetFactionId(GridTile))
			{
				OutGridTiles.Add(GridTile);
			}
		}
	}
	
	return OutGridTiles;
}

TArray<const UConnectIt_PlayerData*> UConnectIt_GameFacade::GetAllPlayerData() const
{
	TArray<const UConnectIt_PlayerData*> OutPlayerData;
	if (!GameState) { return OutPlayerData; }
	for (const auto Data : GameState->AllPlayerData)
	{
		OutPlayerData.Add(Data);
	}
	
	return OutPlayerData;
}

UConnectIt_PlayerData* UConnectIt_GameFacade::GetPlayerDataById(const int32 PlayerId) const
{
	if (!GameState) { return nullptr; }
	if (!GameState->AllPlayerData.IsValidIndex(PlayerId)) { return nullptr; }
	return GameState->AllPlayerData[PlayerId];
}

const UConnectIt_PlayerData* UConnectIt_GameFacade::GetCurrentPlayerData() const
{
	if (!GameState) { return nullptr; }
	const int32 Index = GameState->GameTurnTracker->GetCount() % GameState->AllPlayerData.Num();
	return GameState->AllPlayerData[Index];
}

TArray<float> UConnectIt_GameFacade::GetPlayerScores() const
{
	TArray<float> OutScores;
	if (!GameState) { return OutScores; }
	for (const auto PlayerData : GetAllPlayerData())
	{
		OutScores.Add(PlayerData->Score);
	}

	return OutScores;
}

void UConnectIt_GameFacade::GetConnectionsFromMove(const int32 InPlayerID, const FGridPosition InMove) const
{
	// get tiles with players pieces	
	TArray<FGridPosition> PlayerGridPositions;
	for (const auto GridTile : GetGridTilesWithPlayerPieces(InPlayerID))
	{
		if (FGridPosition OutGridPosition; GetPositionFromTile(OutGridPosition, GridTile))
		{
			PlayerGridPositions.Add(OutGridPosition);			
		}
	}

	// find any connections of min 4 length
	TArray<TArray<FGridPosition>> Outlines;
	UGridMechanics_ShapeLibrary::GetLongestLines(Outlines, PlayerGridPositions , InMove, 4);

	// cumulate score
	TArray<int32> Scores;
	for (const auto Outline : Outlines)
	{
		int32 ScoreTemp = 0;
		for (const auto GridPosition : Outline)
		{
			if (AGridTileBase* OutTile = GetTileFromPosition(GridPosition); IsValid(OutTile))
			{
				ScoreTemp += IGridLevelInterface::Execute_GetGridLevel(OutTile);
			}
		}

		Scores.Add(ScoreTemp);
	}

	int32 TotalScore = 0;
	for (const auto Score : Scores) { TotalScore += Score; }
}

int32 UConnectIt_GameFacade::GetConnectionScore(const TArray<FGridPosition>& InConnection) const
{
	int32 Score = 0;
	for (const auto GridPosition : InConnection)
	{
		if (AGridTileBase* OutTile = GetTileFromPosition(GridPosition);
			IsValid(OutTile) &&	OutTile->Implements<UGridLevelInterface>())
		{
			Score += IGridLevelInterface::Execute_GetGridLevel(OutTile);
		}
	}

	return Score;
}

bool UConnectIt_GameFacade::GetPositionFromTile(FGridPosition& OutGridPosition, const AGridTileBase* InGridTile) const
{
	if (!InGridTile) { return false; }
	OutGridPosition = UGridMechanics_GridLibrary::CalculateGridPositionFromSize(
		InGridTile, UConnectIt_GridRulesLibrary::ConnectIt_Grid_Size_Default);
	return true;
}

AGridTileBase* UConnectIt_GameFacade::GetTileFromPosition(const FGridPosition InGridPosition) const
{
	if (!GameState) { return nullptr; }
	for (const auto GridTile : GetGridTiles())
	{
		if (IGridTileHandler::Execute_GetGridPosition(GridTile) == InGridPosition)
		{
			return GridTile;
		}
	}

	return nullptr;
}

AGridTileBase* UConnectIt_GameFacade::GetRandomGridTile() const
{
	if (!GameState) { return nullptr; }
	return UGridMechanics_GridLibrary::GetRandomGridTile(GetGridTiles());
}

bool UConnectIt_GameFacade::IsTileEmpty(AGridTileBase* InGridTile) const
{
	if (InGridTile == nullptr) { return false; }
	return !IGridPieceHandler::Execute_HasPiece(InGridTile);
}
