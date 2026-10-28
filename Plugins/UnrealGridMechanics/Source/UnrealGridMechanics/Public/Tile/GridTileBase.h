// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interface/GridBoardActor.h"
#include "GridTileBase.generated.h"

class AGridPieceBase;
class UGridHoverComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridTileBaseBeginCursorOver, AGridTileBase*, GridTileBase);

UCLASS()
class UNREALGRIDMECHANICS_API AGridTileBase : public AActor, public IGridBoardActor
{
	GENERATED_BODY()

public:
	AGridTileBase();

	// Raises hover begin / end and click from the tile's cursor events, in
	// C++ (no Blueprint wiring). Every tile class has one.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Tile")
	TObjectPtr<UGridHoverComponent> HoverComponent;

	// Deprecated: forwarded from HoverComponent's hover begin for one release,
	// so existing Blueprints keep working. Don't broadcast it by hand (the
	// component raises hover now); listen to the component or
	// UGridHoverSubsystem instead.
	UPROPERTY(BlueprintAssignable, BlueprintCallable, meta = (DeprecatedProperty, DeprecationMessage =
		"The hover component raises hover now: remove manual broadcasts; listen to HoverComponent or UGridHoverSubsystem."))
	FGridTileBaseBeginCursorOver OnGridTileBeginCursorOver;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Tile")
	void SendGameplayTag(FGameplayTag StateTag);

	// IGridBoardActor: nothing here -- a game's tile reads its own board data
	virtual void RefreshFromBoard_Implementation() override {}

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void ForwardHoverBegin(UGridHoverComponent* Component);
	
};
