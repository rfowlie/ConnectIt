// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "States/GameMechanicsStateSimple.h"
#include "ConnectIt_State_Game.generated.h"

class UActorPool;
class UGameTurnTracker;
class UConnectIt_State_UpdateGameBoard;
class UConnectIt_GameFacade;
class UConnectIt_GameViewModel;
class UConnectIt_State_PlacePiece;
class UConnectIt_State_SelectTile;
class UConnectIt_PlayerData;
class AGridTileBase;

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class CONNECTIT_API UConnectIt_State_Game : public UGameMechanicsStateSimple
{
	GENERATED_BODY()

public:
	UConnectIt_State_Game();
	
	UPROPERTY()
	UConnectIt_State_SelectTile* StatePlayerTurn = nullptr;
	UPROPERTY()
	UConnectIt_State_PlacePiece* StatePlacePiece = nullptr;
	UPROPERTY()
	UConnectIt_State_UpdateGameBoard* StateUpdateBoard = nullptr;

	UPROPERTY()
	UConnectIt_GameFacade* GameFacade = nullptr;
	UPROPERTY()
	UConnectIt_GameViewModel* GameViewModel = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TArray<UConnectIt_PlayerData*> AllPlayerData;
	
	UPROPERTY(BlueprintReadWrite)
	TArray<AGridTileBase*> AllGridTiles;
	
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	UActorPool* PlayerPiecePool;

	UPROPERTY()
	UGameTurnTracker* GameTurnTracker = nullptr;

	// state complete
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt")
	void OnTileSelected();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt")
	void OnPiecePlaced();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt")
	void OnBoardUpdated();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt")
	void StartNextPlayerTurn();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt")
	bool IsGameBoardFull();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt")
	bool CheckPlayerWon(const UConnectIt_PlayerData* InPlayerData);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt")
	void SetWorldContext(UWorld* InWorldContext);
	
protected:
	UPROPERTY(BlueprintReadOnly)
	UWorld* WorldContext = nullptr;
};
