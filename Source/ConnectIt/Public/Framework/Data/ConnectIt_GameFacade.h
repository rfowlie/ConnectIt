// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_PlayerData.h"
#include "GridMechanicsBaseStructs.h"
#include "Tile/GridTileBase.h"
#include "UObject/Object.h"
#include "ConnectIt_GameFacade.generated.h"

class AGridTileBase;
class UConnectIt_PlayerData;
class UConnectIt_State_Game;

/**
 * access information about the game without being able to change anything
 */
UCLASS()
class CONNECTIT_API UConnectIt_GameFacade : public UObject
{
	GENERATED_BODY()

public:
	static UConnectIt_GameFacade* Create(UObject* Outer, UConnectIt_State_Game* InGameState);	

	// GRID
	// TODO: return const array?
	UFUNCTION(BlueprintCallable, Category = "ConnectIt | Facade")
	TArray<AGridTileBase*> GetGridTiles() const;

	UFUNCTION(BlueprintCallable, Category = "ConnectIt | Facade")
	TArray<AGridTileBase*> GetEmptyGridTiles() const;

	UFUNCTION(BlueprintCallable, Category = "ConnectIt | Facade")
	bool IsTileEmpty(AGridTileBase* InGridTile) const;
	
	// TODO: logic messy here, should probably also pass in player data not ID
	UFUNCTION(BlueprintCallable, Category = "ConnectIt | Facade")
	TArray<AGridTileBase*> GetGridTilesWithPlayerPieces(const int32 PlayerID) const;

	UFUNCTION(BlueprintCallable, Category = "ConnectIt | Facade | Util")
	bool GetPositionFromTile(FGridPosition& OutGridPosition, const AGridTileBase* InGridTile) const;
	
	UFUNCTION(BlueprintCallable, Category = "ConnectIt | Facade | Util")
	AGridTileBase* GetTileFromPosition(const FGridPosition InGridPosition) const;
	
	UFUNCTION(BlueprintCallable, Category = "ConnectIt | Facade | Util")
	AGridTileBase* GetRandomGridTile() const;

	// PLAYER
	UFUNCTION(BlueprintCallable, Category = "ConnectIt | Facade")
	TArray<const UConnectIt_PlayerData*> GetAllPlayerData() const;

	UFUNCTION(BlueprintCallable, Category = "ConnectIt | Facade")
	UConnectIt_PlayerData* GetPlayerDataById(const int32 PlayerId) const;
	
	UFUNCTION(BlueprintCallable, Category = "ConnectIt | Facade")
	const UConnectIt_PlayerData* GetCurrentPlayerData() const;
	
	UFUNCTION(BlueprintCallable, Category = "ConnectIt | Facade")
	TArray<float> GetPlayerScores() const;

	// SCORING
	UFUNCTION(BlueprintCallable, Category = "ConnectIt | Facade | Scoring")
	void GetConnectionsFromMove(const int32 InPlayerID, const FGridPosition InMove) const;
	
	UFUNCTION(BlueprintCallable, Category = "ConnectIt | Facade | Scoring")
	int32 GetConnectionScore(const TArray<FGridPosition>& InConnection) const;
	
protected:
	UPROPERTY()
	UConnectIt_State_Game* GameState = nullptr;
	
};
