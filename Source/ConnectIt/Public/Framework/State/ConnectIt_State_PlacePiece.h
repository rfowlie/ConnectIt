// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "States/GameMechanicsStateSimple.h"
#include "ConnectIt_State_PlacePiece.generated.h"

class AGridTileBase;
class UConnectIt_GameFacade;
class UConnectIt_GameViewModel;

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class CONNECTIT_API UConnectIt_State_PlacePiece : public UGameMechanicsStateSimple
{
	GENERATED_BODY()

public:
	static UConnectIt_State_PlacePiece* Create(UObject* Outer, UConnectIt_GameFacade* InGameFacade, UConnectIt_GameViewModel* InViewModel);

	UFUNCTION(BlueprintCallable, Category = "ConnectIt")
	void SetSelectedTile(AGridTileBase* InGridTile) { SelectedTile = InGridTile; }
	
protected:
	UPROPERTY()
	UConnectIt_GameViewModel* GameViewModel = nullptr;
	UPROPERTY()
	UConnectIt_GameFacade* GameFacade = nullptr;

	UPROPERTY(BlueprintReadWrite)
	AGridTileBase* SelectedTile = nullptr;
};
