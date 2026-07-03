// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnrealGridMechanics/Public/Shift/GridShiftTypes.h"
#include "BoardActor.generated.h"

class UGridTileRegistryComponent;
class UBoardStateComponent;
class UBoardShiftComponent;


UCLASS(Blueprintable, BlueprintType)
class UNREALGRIDMECHANICS_API ABoardActor : public AActor
{
	GENERATED_BODY()

public:

	ABoardActor();

	// --- Component Accessors ---

	UFUNCTION(BlueprintPure, Category = "Grid|Board")
	UGridTileRegistryComponent* GetRegistry() const { return RegistryComponent; }

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
	UGridTileRegistryComponent* RegistryComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
		Category = "Grid|Board", meta = (AllowPrivateAccess = "true"))
	UBoardStateComponent* StateComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
		Category = "Grid|Board", meta = (AllowPrivateAccess = "true"))
	UBoardShiftComponent* ShiftComponent = nullptr;
};
