// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "GameFramework/Actor.h"
#include "GridTileBase.generated.h"

class AGridPieceBase;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridTileBaseBeginCursorOver, AGridTileBase*, GridTileBase);

UCLASS()
class UNREALGRIDMECHANICS_API AGridTileBase : public AActor
{
	GENERATED_BODY()

public:
	AGridTileBase();

	// allow for grid actors to customize what triggers the cursor over
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FGridTileBaseBeginCursorOver OnGridTileBeginCursorOver;

	// TODO: DO WE NEED THIS???
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Grid | Tile")
	void PlacePiece(AActor* InGridPiece);


protected:
	virtual void BeginPlay() override;
	
};
