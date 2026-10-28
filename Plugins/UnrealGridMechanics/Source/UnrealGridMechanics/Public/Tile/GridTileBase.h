// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "GridTileBase.generated.h"

class AGridPieceBase;
class UGridHoverComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridTileBaseBeginCursorOver, AGridTileBase*, GridTileBase);

UCLASS()
class UNREALGRIDMECHANICS_API AGridTileBase : public AActor
{
	GENERATED_BODY()

public:
	AGridTileBase();

	// Raises hover begin / end and click from the tile's cursor events, in
	// C++ (no Blueprint wiring). Every tile class has one.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Tile")
	TObjectPtr<UGridHoverComponent> HoverComponent;

	// allow for grid actors to customize what triggers the cursor over
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FGridTileBaseBeginCursorOver OnGridTileBeginCursorOver;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Tile")
	void SendGameplayTag(FGameplayTag StateTag);

protected:
	virtual void BeginPlay() override;
	
};
