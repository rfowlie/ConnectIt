// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "Components/ActorComponent.h"
#include "GridPieceRegistryComponent.generated.h"

class UGridTileRegistryComponent;
class UGridWorldSubsystem;
class AGridTileBase;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNREALGRIDMECHANICS_API UGridPieceRegistryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGridPieceRegistryComponent();

	UFUNCTION(BlueprintPure, Category = "Grid|Registry")
	AGridPieceBase* GetPiece(FGridPosition Position) const;


protected:
	virtual void BeginPlay() override;
	
private:
	// Cached subsystem reference — resolved once on BeginPlay
	UPROPERTY()
	UGridWorldSubsystem* GridSubsystem = nullptr;

	UPROPERTY()
	UGridTileRegistryComponent* GridTileRegistryComponent = nullptr;

	// Resolves and caches the subsystem reference
	bool ResolveSubsystem();
	bool ResolveTileRegistry();
	
};
