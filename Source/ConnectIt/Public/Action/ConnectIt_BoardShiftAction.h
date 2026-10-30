// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseEnums.h"
#include "Action/TurnBasedPlayerAction.h"
#include "Tile/GridTileRegistryBase.h"
#include "ConnectIt_BoardShiftAction.generated.h"

enum class EGridDirection : uint8;
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CONNECTIT_API UConnectIt_BoardShiftAction : public UTurnBasedPlayerAction
{
	GENERATED_BODY()

	UConnectIt_BoardShiftAction();

public:

	// The cursor state of every tile in the hovered line -- same as
	// UConnectIt_PlacePieceAction (default Grid.CursorState.Hovered). Single-tile
	// selection (the whole line shifts from whichever tile is picked) -- not a
	// two-step pick like UConnectIt_SwapPieceAction. No cursor of its own yet.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Tags")
	FGameplayTag TagActionMouseHover;

	// Server-side check that a request's ActionTag matches its OperationTag
	virtual bool ProducesRequestType_Implementation(FGameplayTag OperationTag) const override;

    // Its player action tag (ConnectIt.PlayerAction.*)
    virtual FGameplayTag GetActionTag_Implementation() const override;

	// How many places along the shift direction a shift by this action moves
	// the line (counted in shiftable tiles -- a fixed tile is jumped over, not
	// counted). Fixed per action class: EditDefaultsOnly so the class default
	// object is authoritative, because the Mediator checks every request's
	// Steps against it -- a request can't ask for more than the action grants.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Config", meta = (ClampMin = 1))
	int32 ShiftSteps = 1;


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
