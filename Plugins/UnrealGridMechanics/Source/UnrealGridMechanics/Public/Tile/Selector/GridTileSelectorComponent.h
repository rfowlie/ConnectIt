// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GridMechanics_Structs.h"
#include "GridTileSelectorComponent.generated.h"

class AGridTileBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGridTileSelectorDelegate, AGridTileBase*, GridTile, FGridPosition, GridPosition);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNREALGRIDMECHANICS_API UGridTileSelectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGridTileSelectorComponent();

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FGridTileSelectorDelegate GridTileSelectorDelegate;

};
