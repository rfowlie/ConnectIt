// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ConnectIt_BoardShiftAction.h"
#include "ConnectIt_GameplayTags.h"
#include "ConnectIt_Structs.h"
#include "GridMechanics_GridLibrary.h"
#include "Interface/GridFactionInterface.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "Tile/GridTileBase.h"


UConnectIt_BoardShiftAction::UConnectIt_BoardShiftAction()
{
	// Not yet part of the OR/AND turn-end requirement set -- see
	// _decisions/2026-09-14-turn-end-requirements-need-and-or-groups.md.
	// Setting this true now would make BoardShift silently mandatory every
	// turn under the current OR-pair CanAutoEndTurn override, which isn't
	// the intended behaviour yet.
	bIsRequired = false;
	bIsCancellable = true;

	// The actual fix that makes this action reachable at all -- without
	// this, Activate_Internal_Implementation never binds hover/selection
	// input in the first place.
	bRequiresSelection = true;
	MaxCompletionsPerTurn = 1;
	CooldownTurns = 0;
}

void UConnectIt_BoardShiftAction::PostInitialiseAction_Implementation()
{
	if (!IsValid(OwningController))
	{
		UE_LOG(LogTemp, Error,
			TEXT("PieceSwapperAction: PostInitialiseAction -- "
				 "OwningController is null"));
		return;
	}

	// TileRegistry lives on UConnectIt_BoardRegistrySubsystem -- one
	// canonical per-world instance, not per-controller.
	TileRegistry = UConnectIt_GameUtilityLibrary::GetTileRegistry(GetPlayerController());

	if (!IsValid(TileRegistry))
	{
		UE_LOG(LogTemp, Error,
			TEXT("PieceSwapperAction: PostInitialiseAction -- "
				 "OwningController has no valid TileRegistry"));
	}
}

void UConnectIt_BoardShiftAction::HandleValidHover_Implementation(AGridTileBase* Tile)
{
	if (!IsValid(Tile)) return;
	if (!IsValid(TileRegistry)) return;
	if (!TagActionMouseHover.IsValid()) return;

	for (const auto OutTile : TileRegistry->GetTilesByDirection(
		TileRegistry->GetPositionOfTile(Tile), ShiftDirection))
	{
		// TODO: could we have a ITileShiftVisualizer - SetShiftVisual(EGridDirection)
		OutTile->SendGameplayTag(TagActionMouseHover);
	}
}

void UConnectIt_BoardShiftAction::HandleHoverCleared_Implementation(AGridTileBase* PreviousTile)
{
	if (!IsValid(PreviousTile)) return;
	if (!IsValid(TileRegistry)) return;
	if (!TagActionGridState.IsValid()) return;

	for (const auto OutTile : TileRegistry->GetTilesByDirection(
		TileRegistry->GetPositionOfTile(PreviousTile), ShiftDirection))
	{
		OutTile->SendGameplayTag(TagActionGridState);
	}
}

void UConnectIt_BoardShiftAction::HandleValidSelection_Implementation(AGridTileBase* Tile)
{
	if (!IsValid(Tile)) return;
	if (!IsValid(TileRegistry)) return;

	const FGridPosition Position = TileRegistry->GetPositionOfTile(Tile);

	// GetTilesByDirection iterates the registry's own Tiles array (sorted by
	// actor name, not geometry) -- collect positions, then sort into actual
	// line order below, so the Mediator gets a usable, ordered list instead
	// of having to re-derive order itself.
	TArray<FGridPosition> ShiftPositions;
	for (const auto LineTile : TileRegistry->GetTilesByDirection(Position, ShiftDirection))
	{
		if (IsValid(LineTile))
		{
			ShiftPositions.Add(TileRegistry->GetPositionOfTile(LineTile));
		}
	}

	// this will sort the tiles in the correct grid direction
	const FGridDirectionVector Dir = UGridMechanics_GridLibrary::GetGridDirectionVector(ShiftDirection);
	ShiftPositions.Sort([&Position, &Dir](const FGridPosition& A, const FGridPosition& B)
	{
		// Signed distance along the shift axis from the selected tile --
		// orders the line from one end to the other, not by discovery order.
		const int32 DistA = (A.X - Position.X) * Dir.Row + (A.Y - Position.Y) * Dir.Column;
		const int32 DistB = (B.X - Position.X) * Dir.Row + (B.Y - Position.Y) * Dir.Column;
		return DistA < DistB;
	});

	// Build the request -- the Mediator still re-validates ShiftPositions
	// against its own board state before mutating anything; this action has
	// no knowledge of board mutation, same division of responsibility as
	// PlacePiece/Swap.
	FTurnActionRequest Request;
	Request.RequestType = ConnectIt_Game_Shift;
	Request.FactionID = GetOwningControllerFactionID();
	Request.Payload.InitializeAs<FConnectItRequestBoardShift>(
		FConnectItRequestBoardShift{ .Positions = ShiftPositions, .Direction = ShiftDirection });

	UE_LOG(LogTemp, Log,
		TEXT("BoardShiftAction: Selection confirmed at (%d,%d), direction %d, "
			 "%d tile(s) in line, faction %d — requesting board shift"),
		Position.X, Position.Y,
		static_cast<uint8>(ShiftDirection),
		ShiftPositions.Num(),
		Request.FactionID);

	// Route to action component which sends to server. Complete() is not
	// called here -- UTurnBasedActionsComponent pushes an
	// awaiting-confirmation state and calls Complete() itself once the
	// server's answer arrives, same as PlacePiece/Swap.
	RequestBoardChange(Request);
}

void UConnectIt_BoardShiftAction::ClearSelectionState_Implementation()
{
	if (AGridTileBase* Hovered = CurrentHoveredTile.Get())
	{
		HandleHoverCleared_Implementation(Hovered);
	}
}

int32 UConnectIt_BoardShiftAction::GetOwningControllerFactionID() const
{
	if (!IsValid(OwningController))
	{
		UE_LOG(LogTemp, Error,
			TEXT("BoardShiftAction: GetOwningControllerFactionID -- "
				 "OwningController is null"));
		return -1;
	}

	int32 FactionId = -1;
	if (OwningController->Implements<UGridFactionInterface>())
	{
		FactionId = IGridFactionInterface::Execute_GetFactionId(OwningController);
	}

	return FactionId;
}
