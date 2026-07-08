// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GridMechanicsBaseStructs.h"
#include "Board/Interpreter/BoardStateInterpreter.h"
#include "ConnectIt_Structs.h"
#include "ConnectItTileStateInterpreter.generated.h"


// Reads board state changes and sends GameplayTags to AGridTileBase actors
// Tile Blueprint subclasses respond visually to the tags they receive
// Runs on ALL clients independently -- no server involvement
UCLASS(Blueprintable, ClassGroup=(ConnectIt),
	meta=(BlueprintSpawnableComponent))
class CONNECTIT_API UConnectItTileStateInterpreter : public UBoardStateInterpreter
{
	GENERATED_BODY()

public:

	// GameplayTags sent to tiles -- set in editor per project visual design
	// Tiles respond to these in Blueprint however artists want
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interpreter|Tags")
	FGameplayTag Tag_TileOccupied;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interpreter|Tags")
	FGameplayTag Tag_TileEmpty;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interpreter|Tags")
	FGameplayTag Tag_TileInactive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interpreter|Tags")
	FGameplayTag Tag_MultiplierChanged;

protected:

	virtual void OnBoardStateChanged_Implementation(
		const UBoardStateComponentBase* Component) override;

private:

	void ProcessTileChange(
		const FGridPosition& Position,
		const FConnectItTileData& PreviousData,
		const FConnectItTileData& CurrentData);

	void SendTagToTile(
		const FGridPosition& Position,
		FGameplayTag Tag,
		int32 FactionPiece = -1);

	AGridTileBase* FindTileActor(const FGridPosition& Position) const;
};