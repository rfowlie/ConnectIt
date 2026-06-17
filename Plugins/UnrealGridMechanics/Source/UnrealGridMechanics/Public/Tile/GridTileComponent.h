// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GridTileComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridTileComponentDelegate, UGridTileComponent*, Component);


/*
 * Anything that is a grid tile will be required to have this component
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNREALGRIDMECHANICS_API UGridTileComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGridTileComponent();

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FGridTileComponentDelegate OnGridTileBeginCursorOver;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FGridTileComponentDelegate OnGridTileEndCursorOver;
	
};
