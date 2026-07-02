// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "States/GameMechanicsStateSimple.h"
#include "ConnectIt_State_SelectTile.generated.h"

class AGridTileBase;
class UConnectIt_GameViewModel;
class UConnectIt_GameFacade;

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class CONNECTIT_API UConnectIt_State_SelectTile : public UGameMechanicsStateSimple
{
	GENERATED_BODY()

public:
	static UConnectIt_State_SelectTile* Create(UObject* Outer, UConnectIt_GameFacade* InGameFacade, UConnectIt_GameViewModel* InViewModel);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt | Player Turn")
	AGridTileBase* GetSelectedTile();

	UFUNCTION()
	void CheckSelectedTile(AGridTileBase* GridTile, FGridPosition GridPosition);

	// interface
	virtual void Enter_Implementation() override;
	
	
protected:
	UPROPERTY(BlueprintReadOnly)
	UConnectIt_GameViewModel* GameViewModel = nullptr;
	UPROPERTY(BlueprintReadOnly)
	UConnectIt_GameFacade* GameFacade = nullptr;

	UPROPERTY(BlueprintReadWrite)
	AGridTileBase* SelectedTile = nullptr;
};
