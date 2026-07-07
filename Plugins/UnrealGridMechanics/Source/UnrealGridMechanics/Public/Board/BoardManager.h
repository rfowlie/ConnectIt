// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnrealGridMechanics/Public/Board/Shift/GridShiftTypes.h"
#include "BoardManager.generated.h"

class UGridPieceRegistryComponent;
class UGridTileRegistryComponent;
class UBoardStateComponent;
class UBoardShiftComponent;


UCLASS(Blueprintable, BlueprintType)
class UNREALGRIDMECHANICS_API ABoardManager : public AActor
{
	GENERATED_BODY()

public:

	ABoardManager();

	// --- Component Accessors ---

	UFUNCTION(BlueprintPure, Category = "Grid|Board")
	UGridTileRegistryComponent* GetTileRegistry() const { return TileRegistryComponent; }

	UFUNCTION(BlueprintPure, Category = "Grid|Board")
	UGridPieceRegistryComponent* GetPieceRegistry() const { return PieceRegistryComponent; }
	
	UFUNCTION(BlueprintPure, Category = "Grid|Board")
	UBoardStateComponent* GetBoardState() const { return StateComponent; }

	UFUNCTION(BlueprintPure, Category = "Grid|Board")
	UBoardShiftComponent* GetShiftComponent() const { return ShiftComponent; }

	// --- Convenience Pass-throughs ---

	// Requests a shift — returns false if a shift is in progress
	UFUNCTION(BlueprintCallable, Category = "Grid|Board")
	bool RequestShift(FShiftOperation Operation) const;

	UFUNCTION(BlueprintPure, Category = "Grid|Board")
	bool IsShifting() const;

private:

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
		Category = "Grid|Board", meta = (AllowPrivateAccess = "true"))
	UGridTileRegistryComponent* TileRegistryComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
		Category = "Grid|Board", meta = (AllowPrivateAccess = "true"))
	UGridPieceRegistryComponent* PieceRegistryComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
		Category = "Grid|Board", meta = (AllowPrivateAccess = "true"))
	UBoardStateComponent* StateComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
		Category = "Grid|Board", meta = (AllowPrivateAccess = "true"))
	UBoardShiftComponent* ShiftComponent = nullptr;
};
