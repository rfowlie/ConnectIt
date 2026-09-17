// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseEnums.h"
#include "Action/TurnBasedAction.h"
#include "Tile/GridTileRegistryBase.h"
#include "ConnectIt_BoardShiftAction.generated.h"

enum class EGridDirection : uint8;
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CONNECTIT_API UConnectIt_BoardShiftAction : public UTurnBasedAction
{
	GENERATED_BODY()

	UConnectIt_BoardShiftAction();

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Tags")
	FGameplayTag TagActionGridState;
	
	// GameplayTags sent to tiles to drive their visual state -- same
	// pattern as UConnectIt_PlacePieceAction. Single-tile selection (the
	// whole line shifts from whichever tile is picked) -- not a two-step
	// pick like UConnectIt_SwapPieceAction.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Tags")
	FGameplayTag TagActionMouseHover;


protected:

	virtual void PostInitialiseAction_Implementation() override;
	virtual void HandleValidHover_Implementation(AGridTileBase* Tile) override;
	virtual void HandleHoverCleared_Implementation(AGridTileBase* PreviousTile) override;
	virtual void HandleValidSelection_Implementation(AGridTileBase* Tile) override;
	virtual void ClearSelectionState_Implementation() override;

	UPROPERTY()
	TObjectPtr<UGridTileRegistryBase> TileRegistry = nullptr;

	// for controlling and visualizing the grid shift
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Config")
	EGridDirection ShiftDirection = EGridDirection::Up;

private:

	// Gets owning faction ID from participant component slot index --
	// same pattern as UConnectIt_PlacePieceAction/UConnectIt_SwapPieceAction.
	int32 GetOwningControllerFactionID() const;
};
