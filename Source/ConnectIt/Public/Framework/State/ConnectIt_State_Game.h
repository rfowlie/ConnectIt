// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_State_Base.h"
#include "ConnectIt_State_PlacePiece.h"
#include "ConnectIt_State_SelectTile.h"
#include "ConnectIt_State_UpdateGameBoard.h"
#include "GameplayTagContainer.h"
#include "ConnectIt_State_Game.generated.h"

class AConnectIt_BoardManager;
class UActorPool;
class UGameTurnTracker;
class UConnectIt_State_UpdateGameBoard;
class UConnectIt_GameFacade;
class UConnectIt_GameViewModel;
class UConnectIt_State_PlacePiece;
class UConnectIt_State_SelectTile;
class UConnectIt_PlayerData;
class AGridTileBase;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FConnectItGameStateDelegate, FGameplayTag, Tag);

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class CONNECTIT_API UConnectIt_State_Game : public UConnectIt_State_Base
{
	GENERATED_BODY()

public:
	UConnectIt_State_Game();
	
	UFUNCTION(BlueprintCallable, Category = "ConnectIt | State")
	static UConnectIt_State_Game* Create(UObject* Outer);

	UFUNCTION(BlueprintCallable, Category = "ConnectIt | State")
	void Initialize(UObject* Outer);
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FConnectItGameStateDelegate OnGameStateChanged;
	
	
	UPROPERTY(BlueprintReadOnly)
	UConnectIt_State_SelectTile* StatePlayerTurn = nullptr;
	UPROPERTY(BlueprintReadOnly)
	UConnectIt_State_PlacePiece* StatePlacePiece = nullptr;
	UPROPERTY(BlueprintReadOnly)
	UConnectIt_State_UpdateGameBoard* StateUpdateBoard = nullptr;

	UPROPERTY(BlueprintReadOnly)
	UConnectIt_GameFacade* GameFacade = nullptr;
	UPROPERTY(BlueprintReadOnly)
	UConnectIt_GameViewModel* GameViewModel = nullptr;

	UPROPERTY(BlueprintReadWrite, meta=(ExposeOnSpawn = "true"))
	AConnectIt_BoardManager* BoardManager = nullptr;
	
	UPROPERTY(BlueprintReadWrite, meta=(ExposeOnSpawn = "true"))
	TArray<UConnectIt_PlayerData*> AllPlayerData;
	
	UPROPERTY(BlueprintReadWrite, meta=(ExposeOnSpawn = "true"))
	TArray<AGridTileBase*> AllGridTiles;
	
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	UActorPool* PlayerPiecePool = nullptr;

	UPROPERTY(BlueprintReadOnly)
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

	
protected:
	// UPROPERTY(BlueprintReadOnly, meta=(ExposeOnSpawn = "true"))
	// UWorld* WorldContext = nullptr;

	// init properties
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UConnectIt_State_SelectTile> StateSelectTileClass = UConnectIt_State_SelectTile::StaticClass();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UConnectIt_State_PlacePiece> StatePlacePieceClass = UConnectIt_State_PlacePiece::StaticClass();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UConnectIt_State_UpdateGameBoard> StateUpdateGameBoardClass = UConnectIt_State_UpdateGameBoard::StaticClass();
	
	void BroadCastGameState(UConnectIt_State_Base* GameState) const;
};
