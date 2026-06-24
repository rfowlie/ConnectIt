// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemInterface.h"
#include "GridMechanicsBaseEnums.h"
#include "Interface/GridSizeInterface.h"
#include "GridUnitBase.generated.h"


UCLASS(Blueprintable, BlueprintType)
class UNREALGRIDMECHANICS_API AGridUnitBase : public AActor, public IAbilitySystemInterface, public IGridSizeInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGridUnitBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid Mechanics|Grid Unit")
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Grid Mechanics|Grid Unit")
	FGameplayTag GetFactionTag();

	// rotate and move all relevant meshes to face set direction
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Grid Mechanics|Grid Unit")
	void FaceGridDirection(const EGridDirection InDirection);
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UAbilitySystemComponent* AbilitySystemComponent = nullptr;
};
