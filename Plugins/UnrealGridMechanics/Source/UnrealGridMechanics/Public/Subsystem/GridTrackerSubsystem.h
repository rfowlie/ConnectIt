// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GridTrackerSubsystem.generated.h"

class AGridTileBase;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridTrackerTileDelegate, AGridTileBase*, GridTile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridTrackerUnitDelegate, AGridUnitBase*, GridUnit);

/**
 * 
 */
UCLASS(BlueprintType)
class UNREALGRIDMECHANICS_API UGridTrackerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
	virtual void PostInitialize() override;
	
# pragma region Tile
public:
	UPROPERTY(BlueprintAssignable, Category="Turn Based | Grid")
	FGridTrackerTileDelegate OnGridTileHoveredStart;
	UPROPERTY(BlueprintAssignable, Category="Turn Based | Grid")
	FGridTrackerTileDelegate OnGridTileHoveredStop;
	UFUNCTION(BlueprintCallable)
	AGridTileBase* GetGridTileHovered() const { return GridTileHovered; }

	UFUNCTION()
	void OnBeginCursorOverGridTile(AGridTileBase* GridTileBase);
	UFUNCTION(BlueprintCallable)
	void RegisterGridTile(AGridTileBase* GridTile);
	UFUNCTION(BlueprintCallable)
	void UnregisterGridTile(AGridTileBase* GridTile);
	UFUNCTION(BlueprintCallable)
	TArray<AGridTileBase*> GetAllGridTiles() { return GridTilesAll; }

private:
	UPROPERTY()
	TArray<AGridTileBase*> GridTilesAll;
	UPROPERTY()
	TObjectPtr<AGridTileBase> GridTileHovered;

# pragma endregion
	
# pragma region Unit
public:
	// UPROPERTY(BlueprintAssignable, Category="Turn Based | Grid")
	// FGridTrackerUnitDelegate OnGridUnitHoveredStart;
	// UPROPERTY(BlueprintAssignable, Category="Turn Based | Grid")
	// FGridTrackerUnitDelegate OnGridUnitHoveredStop;
	// UFUNCTION(BlueprintCallable)
	// AGridUnitBase* GetGridUnitHovered() const { return GridUnitHovered; }
	
	UFUNCTION(BlueprintCallable)
	void RegisterGridUnit(AGridUnitBase* GridUnit);
	UFUNCTION(BlueprintCallable)
	void UnregisterGridUnit(AGridUnitBase* GridUnit);
	UFUNCTION(BlueprintCallable)
	TArray<AGridUnitBase*> GetAllGridUnits() { return GridUnitsAll; }

private:
	UPROPERTY()
	TArray<AGridUnitBase*> GridUnitsAll;
	UPROPERTY()
	TObjectPtr<AGridUnitBase> GridUnitHovered = nullptr;

#pragma endregion
	
};
