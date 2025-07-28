// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridTileBase.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridTileBaseBeginCursorOver, AGridTileBase*, GridTileBase);

UCLASS()
class UNREALGRIDMECHANICS_API AGridTileBase : public AActor
{
	GENERATED_BODY()

public:
	AGridTileBase();

protected:
	virtual void BeginPlay() override;

public:
	// allow for grid actors to customize what triggers the cursor over
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FGridTileBaseBeginCursorOver OnGridTileBeginCursorOver;
	
};
