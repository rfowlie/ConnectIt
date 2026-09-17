// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ConnectIt_SwapPieceAction.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "ConnectIt_GameplayTags.h"
#include "ConnectIt_Structs.h"
#include "Interface/GridFactionInterface.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridTileRegistryBase.h"


UConnectIt_SwapPieceAction::UConnectIt_SwapPieceAction()
{
    // Alternate-mandatory with PlacePiece -- required (so it counts toward
    // turn-end via UConnectIt_TurnBasedActionsComponent's OR'd pair), but
    // completing it satisfies the turn on its own; no PlacePiece needed
    // afterward that turn. See that class's header comment.
    bIsRequired = true;
    bIsCancellable = true;
    bRequiresSelection = true;
    MaxCompletionsPerTurn = 1;
    CooldownTurns = 0;
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
    if (!TagActionMouseHover.IsValid()) return;
    Tile->SendGameplayTag(TagActionMouseHover);
}

void UConnectIt_SwapPieceAction::HandleHoverCleared_Implementation(
    AGridTileBase* PreviousTile)
{
    if (!IsValid(PreviousTile)) return;

    // The first-picked tile isn't reverting to default just because hover
    // moved off it while awaiting the second pick -- keep its distinct
    // "already picked" highlight instead.
    if (bHasSelectionFirst && IsValid(TileRegistry) &&
        TileRegistry->GetPositionOfTile(PreviousTile) == PositionFirst)
    {
        if (TagFirstSelection.IsValid())
        {
            PreviousTile->SendGameplayTag(TagFirstSelection);
        }
        return;
    }

    if (!TagActionGridState.IsValid()) return;
    PreviousTile->SendGameplayTag(TagActionGridState);
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

        if (TagFirstSelection.IsValid())
        {
            Tile->SendGameplayTag(TagFirstSelection);
        }

        UE_LOG(LogTemp, Log,
            TEXT("PieceSwapperAction: First selection at (%d,%d) -- "
                 "awaiting second tile"),
            Position.X, Position.Y);
        return;
    }

    // Second pick confirmed -- build the request. Board manager handles all
    // validation/mutation; this action has no knowledge of it.
    FTurnActionRequest Request;
    Request.RequestType = ConnectIt_Game_SwapPieces;
    Request.FactionID = GetOwningControllerFactionID();
    Request.Payload.InitializeAs<FConnectItRequestSwapPieces>(
        FConnectItRequestSwapPieces{ .PositionA = PositionFirst, .PositionB = Position });

    UE_LOG(LogTemp, Log,
        TEXT("PieceSwapperAction: Second selection at (%d,%d) -- "
             "requesting swap with (%d,%d), faction %d"),
        Position.X, Position.Y,
        PositionFirst.X, PositionFirst.Y,
        Request.FactionID);

    // Reset selection state before the round trip -- if the server rejects
    // this, the action reactivates via the normal awaiting-confirmation
    // flow and should start fresh, not stuck mid-pick.
    bHasSelectionFirst = false;
    PositionFirst = FGridPosition();

    // Route to action component which sends to server. Complete() is not
    // called here -- UTurnBasedActionsComponent pushes an
    
    // awaiting-confirmation state and calls Complete() itself once the
    // server's answer arrives (NotifyBoardChangeOutcome), same as
    // UConnectIt_PlacePieceAction.
    RequestBoardChange(Request);
}

void UConnectIt_SwapPieceAction::ClearSelectionState_Implementation()
{
    // Clear hover state on currently hovered tile
    if (AGridTileBase* Hovered = CurrentHoveredTile.Get())
    {
        if (TagActionGridState.IsValid())
        {
            Hovered->SendGameplayTag(TagActionGridState);
        }
    }

    // Also clear the first-picked tile's distinct highlight, if any
    if (bHasSelectionFirst && IsValid(TileRegistry) && TagActionGridState.IsValid())
    {
        if (AGridTileBase* FirstTile = TileRegistry->GetTileAtPosition(PositionFirst))
        {
            FirstTile->SendGameplayTag(TagActionGridState);
        }
    }

    bHasSelectionFirst = false;
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