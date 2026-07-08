// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/State/ConnectIt_State_SelectTile.h"

#include "ConnectIt_GameplayTags.h"
#include "Framework/Data/ConnectIt_GameFacade.h"
#include "Framework/Data/ConnectIt_PlayerData.h"
#include "GameTurn/GameTurnParticipant.h"
#include "GameTurn/GameTurnParticipantComponent.h"
#include "Tile/Selector/GridTileSelector.h"
#include "Tile/Selector/GridTileSelectorComponent.h"


UConnectIt_State_SelectTile::UConnectIt_State_SelectTile()
{
	TileSelectorDelegate.BindDynamic(this, &UConnectIt_State_SelectTile::CheckSelectedTile);
}

UConnectIt_State_SelectTile* UConnectIt_State_SelectTile::Create(
	UObject* Outer, UConnectIt_GameFacade* InGameFacade, UConnectIt_GameViewModel* InViewModel)
{
	UConnectIt_State_SelectTile* Obj = NewObject<UConnectIt_State_SelectTile>();
	Obj->GameStateTag = ConnectIt_Game_SelectTile;
	Obj->GameFacade = InGameFacade;
	Obj->GameViewModel = InViewModel;
	return Obj;
}

AGridTileBase* UConnectIt_State_SelectTile::GetSelectedTile_Implementation()
{
	return SelectedTile ? SelectedTile : nullptr;
}

void UConnectIt_State_SelectTile::CheckSelectedTile_Implementation(AGridTileBase* GridTile, FGridPosition GridPosition)
{
	if (GameFacade->IsTileEmpty(GridTile))
	{
		const UConnectIt_PlayerData* Data = GameFacade->GetCurrentPlayerData();
		checkf(Data, TEXT("ConnectIt State - Select Tile - Player Data nullptr "))
		// check(Data->PlayerObject == nullptr);
		// check(Data->PlayerObject->Implements<UGridTileSelector>());
		IGridTileSelector::Execute_SelectGridTileUnregister(Data->PlayerObject, TileSelectorDelegate);
		// Data->GameTurnParticipantComponent->OnTurnEnd.Broadcast();
		Data->GameTurnParticipant->OnTurnEnd.Broadcast(GameStateTag);

		
		// UGridTileSelectorComponent* Component = IGridTileSelector::Execute_GetGridTileSelectorComponent(Data->PlayerObject);
		// Component->GridTileSelectorDelegate.RemoveDynamic(this, &ThisClass::CheckSelectedTile);
		// IGridTileSelector::Execute_SelectGridTileRegister(Data->PlayerObject, &ThisClass::CheckSelectedTile);
		// IGridTileSelector::Execute_SelectGridTileExit(Data->PlayerObject);
		
		SelectedTile = GridTile;
		if (OnStateComplete.IsBound()) { OnStateComplete.Broadcast(); }
	}
}

void UConnectIt_State_SelectTile::Enter_Implementation()
{
	const UConnectIt_PlayerData* Data = GameFacade->GetCurrentPlayerData();
	checkf(Data, TEXT("ConnectIt State - Select Tile - Player Data nullptr "))
	// check(Data->PlayerObject == nullptr);
	// check(Data->PlayerObject->Implements<UGridTileSelector>());
	IGridTileSelector::Execute_SelectGridTileRegister(Data->PlayerObject, TileSelectorDelegate);
	// Data->GameTurnParticipantComponent->OnTurnBegin.Broadcast();
	if (Data->GameTurnParticipant) { Data->GameTurnParticipant->OnTurnBegin.Broadcast(GameStateTag); }

	
	// UGridTileSelectorComponent* Component = IGridTileSelector::Execute_GetGridTileSelectorComponent(Data->PlayerObject);
	// checkf(Component, TEXT("ConnectIt State - Select Tile - Grid Tile Selector Component nullptr "))
	// Component->GridTileSelectorDelegate.AddDynamic(this, &ThisClass::CheckSelectedTile);
	// IGridTileSelector::Execute_SelectGridTileEnter(Data->PlayerObject);
}

