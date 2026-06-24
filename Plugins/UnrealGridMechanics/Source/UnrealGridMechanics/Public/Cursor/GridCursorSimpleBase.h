// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridCursorSimpleBase.generated.h"

class AGridTileBase;


UCLASS(Blueprintable, BlueprintType)
class UNREALGRIDMECHANICS_API AGridCursorSimpleBase : public AActor
{
	GENERATED_BODY()

public:
	AGridCursorSimpleBase();
	
protected:
	virtual void BeginPlay() override;
	
public:	
	// stop updating and hide actor
	UFUNCTION(BlueprintCallable, Category = "Grid Mechanics | Cursor")
	void EnableCursor(bool bValue);

	// stop updating but keep actor visible
	UFUNCTION(BlueprintCallable, Category = "Grid Mechanics | Cursor")
	void PauseCursor(bool bValue);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Mechanics | Cursor")
	FVector Cursor_ExtraHeight = FVector(0.f, 0.f, 5.f);
	
protected:
	UFUNCTION(BlueprintCallable, Category = "Grid Mechanics | Cursor")
	void UpdateCursor(AGridTileBase* GridTile);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* StaticMeshComponent = nullptr;
	
	UPROPERTY(BlueprintReadWrite, Category = "Grid Mechanics | Cursor")
	bool IsEnabled = true;
	
};
