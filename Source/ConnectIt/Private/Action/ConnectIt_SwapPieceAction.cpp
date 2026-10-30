// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ConnectIt_SwapPieceAction.h"
#include "Board/Operations/ConnectIt_BoardOperations.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "ConnectIt_GameplayTags.h"
#include "ConnectIt_Structs.h"
#include "Interface/GridFactionInterface.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridCursorStateTags.h"
#include "Tile/GridTileRegistryBase.h"


bool UConnectIt_SwapPieceAction::ProducesRequestType_Implementation(FGameplayTag OperationTag) const
{
    return OperationTag == ConnectIt_BoardOperation_SwapPieces;
}

FGameplayTag UConnectIt_SwapPieceAction::GetActionTag_Implementation() const
{
    return ConnectIt_PlayerAction_SwapPiece;
}

UConnectIt_SwapPieceAction::UConnectIt_SwapPieceAction()
{
    // Its match and turn uses (a PermanentActions entry) and whether completing it
    // ends the turn (a leaf in the loadout's TurnEndRequirements tree, e.g.
    // an Any group with PlacePiece) are configured in the loadout, not here.
    bIsCancellable = true;
    bRequiresSelection = true;

    TagActionMouseHover = Grid_CursorState_Hovered;
    TagFirstSelection = Grid_CursorState_Selected;
}

void UConnectIt_SwapPieceAction::Deactivate_Internal_Implementation()
{
    Super::Deactivate_Internal_Implementation();

    if (!IsValid(TileRegistry))
    {
        UE_LOG(LogTemp, Error,
               TEXT("PieceSwapperAction: OnCompleted_Implementation -- "
                   "OwningController has no valid TileRegistry"));
    }

    bHasSelectionFirst = false;
    PositionFirst = FGridPosition();
    MarkerCursor = nullptr;   // released with the action's cursors
}

void UConnectIt_SwapPieceAction::PostInitialiseAction_Implementation()
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

bool UConnectIt_SwapPieceAction::IsValidHoverTile_Implementation(AGridTileBase* Tile) const
{
    if (!IsValid(TileRegistry)) return false;
    if (!IsValid(Tile)) return false;

    const UConnectIt_BoardStateComponent* BoardState =
        UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    if (!IsValid(BoardState)) return false;

    const FGridPosition Position = TileRegistry->GetPositionOfTile(Tile);
    const FConnectItTileData* TileData = BoardState->GetCurrentState().GetTileData(Position);
    if (!TileData || !TileData->bIsOccupied) return false;

    if (!bHasSelectionFirst)
    {
        // First pick must be one of the acting player's own pieces -- a UX
        // prefilter only. The server independently re-validates "exactly
        // one side is mine" regardless of what's allowed to be clicked here.
        return TileData->FactionPiece == GetOwningControllerFactionID();
    }

    // Second pick -- any other occupied tile.
    return Position != PositionFirst;
}

bool UConnectIt_SwapPieceAction::IsValidSelectionTile_Implementation(AGridTileBase* Tile) const
{
    // Selection validity matches hover validity
    if (!IsValidHoverTile_Implementation(Tile)) return false;

    return Super::IsValidSelectionTile_Implementation(Tile);
}

void UConnectIt_SwapPieceAction::HandleValidHover_Implementation(AGridTileBase* Tile)
{
    if (!IsValid(Tile)) return;

    SetTileCursorState(Tile, TagActionMouseHover);
    ShowHoverCursor(GetHoverCursorClassFor(Tile), Tile);
}

void UConnectIt_SwapPieceAction::HandleHoverCleared_Implementation(
    AGridTileBase* PreviousTile)
{
    HideHoverCursor();
    if (!IsValid(PreviousTile)) return;

    // The first-picked tile keeps its "already picked" state while hover
    // moves on (it's never a valid hover itself, so nothing replaced it)
    if (PreviousTile == GetFirstTile()) return;

    SetTileCursorState(PreviousTile, FGameplayTag());
}

void UConnectIt_SwapPieceAction::HandleInvalidHover_Implementation(AGridTileBase* Tile)
{
    ShowHoverCursor(InvalidCursorClass, Tile);
}

AGridTileBase* UConnectIt_SwapPieceAction::GetFirstTile() const
{
    return bHasSelectionFirst && IsValid(TileRegistry) ? TileRegistry->GetTileAtPosition(PositionFirst) : nullptr;
}

TSubclassOf<AActor> UConnectIt_SwapPieceAction::GetHoverCursorClassFor(AGridTileBase* Tile) const
{
    const UConnectIt_BoardStateComponent* BoardState =
        UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    if (!IsValid(BoardState) || !IsValid(TileRegistry) || !IsValid(Tile)) return OpponentPieceCursorClass;

    const FConnectItTileData* TileData = BoardState->GetCurrentState().GetTileData(TileRegistry->GetPositionOfTile(Tile));
    return TileData && TileData->FactionPiece == GetOwningControllerFactionID()
        ? OwnPieceCursorClass : OpponentPieceCursorClass;
}

void UConnectIt_SwapPieceAction::ClearFirstSelection()
{
    SetTileCursorState(GetFirstTile(), FGameplayTag());
    HideCursor(MarkerCursor);
    bHasSelectionFirst = false;
    PositionFirst = FGridPosition();
}

void UConnectIt_SwapPieceAction::HandleValidSelection_Implementation(AGridTileBase* Tile)
{
    if (!IsValid(Tile)) return;
    if (!IsValid(TileRegistry)) return;    

    const FGridPosition Position = TileRegistry->GetPositionOfTile(Tile);

    if (!bHasSelectionFirst)
    {
        PositionFirst = Position;
        bHasSelectionFirst = true;

        SetTileCursorState(Tile, TagFirstSelection);
        if (MarkerCursorClass)
        {
            if (!HoldsActionCursor(MarkerCursor))
            {
                MarkerCursor = AcquireActionCursor(MarkerCursorClass);
            }
            ShowCursorAt(MarkerCursor, Tile, /*bReactivate=*/true);
        }

        UE_LOG(LogTemp, Log,
            TEXT("PieceSwapperAction: First selection at (%d,%d) -- "
                 "awaiting second tile"),
            Position.X, Position.Y);
        return;
    }

    // Second pick confirmed -- build the request. Board manager handles all
    // validation/mutation; this action has no knowledge of it.
    FConnectItBoardOperation_SwapPieces Operation;
    Operation.Faction = GetOwningControllerFactionID();
    Operation.PositionA = PositionFirst;
    Operation.PositionB = Position;

    UE_LOG(LogTemp, Log,
        TEXT("PieceSwapperAction: Second selection at (%d,%d) -- "
             "requesting swap with (%d,%d), faction %d"),
        Position.X, Position.Y,
        PositionFirst.X, PositionFirst.Y,
        Operation.Faction);

    // Reset selection state before the round trip -- if the server rejects
    // this, the action reactivates via the normal awaiting-confirmation
    // flow and should start fresh, not stuck mid-pick.
    ClearFirstSelection();

    // Route to action component which sends to server. Complete() is not
    // called here -- UTurnBasedActionsComponent pushes an
    
    // awaiting-confirmation state and calls Complete() itself once the
    // server's answer arrives (NotifyBoardChangeOutcome), same as
    // UConnectIt_PlacePieceAction.
    RequestBoardChange(FInstancedStruct::Make(Operation));
}

void UConnectIt_SwapPieceAction::ClearSelectionState_Implementation()
{
    // Clear the hovered tile, then the first pick's mark, if any
    HideHoverCursor();
    SetTileCursorState(CurrentHoveredTile.Get(), FGameplayTag());
    ClearFirstSelection();
}

int32 UConnectIt_SwapPieceAction::GetOwningControllerFactionID() const
{
    if (!IsValid(OwningController))
    {
        UE_LOG(LogTemp, Error,
            TEXT("PieceSwapperAction: GetOwningControllerFactionID -- "
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