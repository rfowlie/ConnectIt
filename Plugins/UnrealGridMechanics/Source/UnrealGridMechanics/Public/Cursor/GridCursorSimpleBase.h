// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Cursor/GridCursorManagerBase.h"
#include "GridCursorSimpleBase.generated.h"

class AGridTileBase;

// Simple concrete cursor -- moves a static mesh to the hovered tile's
// location. Reference implementation of AGridCursorManagerBase
UCLASS(Blueprintable, BlueprintType)
class UNREALGRIDMECHANICS_API AGridCursorSimpleBase : public AGridCursorManagerBase
{
	GENERATED_BODY()

public:
	AGridCursorSimpleBase();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Mechanics|Cursor")
	FVector Cursor_ExtraHeight = FVector(0.f, 0.f, 5.f);

protected:
	virtual void BeginPlay() override;

	virtual void UpdateCursor_Implementation(AGridTileBase* Tile) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* StaticMeshComponent = nullptr;
};
