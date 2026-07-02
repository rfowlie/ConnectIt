// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ConnectIt_GameViewModel.generated.h"

class UConnectIt_State_Game;
class UConnectIt_PlayerData;
class AGridTileBase;

/**
 * 
 */
UCLASS()
class CONNECTIT_API UConnectIt_GameViewModel : public UObject
{
	GENERATED_BODY()

public:
	static UConnectIt_GameViewModel* Create(UObject* Outer, UConnectIt_State_Game* InGameState);

	// TODO: this will need to work with animations etc...
	UFUNCTION(BlueprintCallable, Category = "ConnectIt | ViewModel")
	void PlacePiece(UConnectIt_PlayerData* InPlayerData, AGridTileBase* InGridTile);

	UFUNCTION(BlueprintCallable, Category = "ConnectIt | ViewModel")
	void SetPlayerScore(const int32 PlayerId, const int32 InScore);
	
protected:
	UPROPERTY()
	UConnectIt_State_Game* GameState = nullptr;
};
