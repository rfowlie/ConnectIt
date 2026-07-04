// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_State_Base.h"
#include "GridMechanicsBaseStructs.h"
#include "Tile/Selector/GridTileSelector.h"
#include "ConnectIt_State_SelectTile.generated.h"

class AGridTileBase;
class UConnectIt_GameViewModel;
class UConnectIt_GameFacade;

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class CONNECTIT_API UConnectIt_State_SelectTile : public UConnectIt_State_Base
{
	GENERATED_BODY()

public:
	UConnectIt_State_SelectTile();
	
	static UConnectIt_State_SelectTile* Create(UObject* Outer, UConnectIt_GameFacade* InGameFacade, UConnectIt_GameViewModel* InViewModel);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt | SelectTile")
	AGridTileBase* GetSelectedTile();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt | SelectTile")
	void CheckSelectedTile(AGridTileBase* GridTile, FGridPosition GridPosition);

	// interface
	virtual void Enter_Implementation() override;
	
	
protected:
	UPROPERTY(BlueprintReadWrite)
	AGridTileBase* SelectedTile = nullptr;

	UPROPERTY()
	FGridTileSelectorDelegate TileSelectorDelegate;
};
