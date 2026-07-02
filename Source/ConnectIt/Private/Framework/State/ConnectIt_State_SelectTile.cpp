// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/State/ConnectIt_State_SelectTile.h"
#include "Framework/Data/ConnectIt_GameFacade.h"
#include "Framework/Data/ConnectIt_PlayerData.h"
#include "Tile/Selector/GridTileSelector.h"
#include "Tile/Selector/GridTileSelectorComponent.h"


UConnectIt_State_SelectTile* UConnectIt_State_SelectTile::Create(
	UObject* Outer, UConnectIt_GameFacade* InGameFacade, UConnectIt_GameViewModel* InViewModel)
{
	UConnectIt_State_SelectTile* State = NewObject<UConnectIt_State_SelectTile>();
	State->GameFacade = InGameFacade;
	State->GameViewModel = InViewModel;
	return State;
}

void UConnectIt_State_SelectTile::CheckSelectedTile(AGridTileBase* GridTile, FGridPosition GridPosition)
{
	if (GameFacade->IsTileEmpty(GridTile))
	{
		const UConnectIt_PlayerData* Data = GameFacade->GetCurrentPlayerData();
		UGridTileSelectorComponent* Component = IGridTileSelector::Execute_GetGridTileSelectorComponent(Data->PlayerObject);
		Component->GridTileSelectorDelegate.RemoveDynamic(this, &ThisClass::CheckSelectedTile);
		IGridTileSelector::Execute_SelectGridTileExit(Data->PlayerObject);
		
		SelectedTile = GridTile;
		if (OnStateComplete.IsBound()) { OnStateComplete.Broadcast(); }
	}
}

AGridTileBase* UConnectIt_State_SelectTile::GetSelectedTile_Implementation()
{
	return SelectedTile ? SelectedTile : nullptr;
}

void UConnectIt_State_SelectTile::Enter_Implementation()
{
	const UConnectIt_PlayerData* Data = GameFacade->GetCurrentPlayerData();
	UGridTileSelectorComponent* Component = IGridTileSelector::Execute_GetGridTileSelectorComponent(Data->PlayerObject);
	Component->GridTileSelectorDelegate.AddDynamic(this, &ThisClass::CheckSelectedTile);
	IGridTileSelector::Execute_SelectGridTileEnter(Data->PlayerObject);
}

