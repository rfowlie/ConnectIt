// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ConnectIt_PlacePieceAction.h"
#include "Board/ConnectIt_BoardManager.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "ConnectIt_Structs.h"
#include "Interface/GridFactionInterface.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridTileRegistryComponent.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"


UConnectIt_PlacePieceAction::UConnectIt_PlacePieceAction()
{
    // Required -- turn cannot end without placing a piece
    bIsRequired = true;

    // Cancellable -- shard or power activation cancels this,
    // and it reactivates after the optional action completes
    bIsCancellable = true;

    bRequiresSelection = true;
    MaxCompletionsPerTurn = 1;
    CooldownTurns = 0;
}

void UConnectIt_PlacePieceAction::PostInitialiseAction_Implementation()
{
    if (!IsValid(OwningController))
    {
        UE_LOG(LogTemp, Error,
            TEXT("PlacePieceAction: PostInitialiseAction -- "
                 "OwningController is null"));
        return;
    }
    
    BoardManager = UConnectIt_GameUtilityLibrary::GetBoardManager(OwningController);

    if (!IsValid(BoardManager))
    {
        UE_LOG(LogTemp, Error,
            TEXT("PlacePieceAction: PostInitialiseAction -- "
                 "no AConnectIt_BoardManager found"));
    }
}

void UConnectIt_PlacePieceAction::Activate_Internal_Implementation()
{
    Super::Activate_Internal_Implementation();
    UE_LOG(LogTemp, Log,
        TEXT("PlacePieceAction: Activated — awaiting tile selection"));
}

void UConnectIt_PlacePieceAction::OnCancelled_Implementation()
{
    Super::OnCancelled_Implementation();
    UE_LOG(LogTemp, Log,
        TEXT("PlacePieceAction: Cancelled — "
             "optional action taking priority"));
}

void UConnectIt_PlacePieceAction::OnCompleted_Implementation()
{
    Super::OnCompleted_Implementation();
    UE_LOG(LogTemp, Log,
        TEXT("PlacePieceAction: Piece placed — action complete"));
}

bool UConnectIt_PlacePieceAction::IsValidHoverTile_Implementation(AGridTileBase* Tile) const
{
    if (!IsValid(BoardManager)) return false;
    if (!IsValid(Tile)) return false;    
   
    const UConnectIt_BoardStateComponent* BoardState = BoardManager->GetBoardStateComponent();
    if (!IsValid(BoardState)) return false;

    // Position is not stored on the tile itself -- resolved via the
    // board manager's tile registry
    FGridPosition Position = BoardManager->GetTileRegistry()->GetPositionOfTile(Tile);

    // Tile must be valid for placement in current board state
    // IsTileValidForPlacement checks both bIsActive and !IsOccupied
    return BoardState->GetCurrentState().IsTileValidForPlacement(Position);
}

bool UConnectIt_PlacePieceAction::IsValidSelectionTile_Implementation(AGridTileBase* Tile) const
{
    // Selection validity matches hover validity
    if (!IsValidHoverTile_Implementation(Tile)) return false;

    return Super::IsValidSelectionTile_Implementation(Tile);
}

void UConnectIt_PlacePieceAction::HandleValidHover_Implementation(AGridTileBase* Tile)
{
    if (!IsValid(Tile)) return;
    if (!Tag_ValidHover.IsValid()) return;

    Tile->SendGameplayTag(Tag_ValidHover);
}

void UConnectIt_PlacePieceAction::HandleHoverCleared_Implementation(
    AGridTileBase* PreviousTile)
{
    if (!IsValid(PreviousTile)) return;
    if (!Tag_Default.IsValid()) return;

    PreviousTile->SendGameplayTag(Tag_Default);
}

void UConnectIt_PlacePieceAction::HandleValidSelection_Implementation(AGridTileBase* Tile)
{
    if (!IsValid(BoardManager)) return;
    if (!IsValid(Tile)) return;    

    FGridPosition Position = BoardManager->GetTileRegistry()->GetPositionOfTile(Tile);

    // Build the request -- board manager handles all mutation
    // Action has no knowledge of pools, piece actors, or state changes
    FTurnActionRequest Request;
    Request.RequestType = ActionTag;
    Request.FactionID = GetOwningControllerFactionID();
    Request.Payload.InitializeAs<FConnectItRequestPlacePiece>(
        FConnectItRequestPlacePiece{ .Positions = { Position } });

    UE_LOG(LogTemp, Log,
        TEXT("PlacePieceAction: Selection confirmed at (%d,%d) "
             "faction %d — requesting board change"),
        Position.X,
        Position.Y,
        Request.FactionID);

    // Route to action component which sends to server. Complete() is no
    // longer called here -- UTurnBasedActionsComponent pushes an
    // awaiting-confirmation state and calls Complete() itself once the
    // server's answer arrives (NotifyBoardChangeOutcome), so a rejected
    // request no longer falsely completes this action.
    RequestBoardChange(Request);
}

void UConnectIt_PlacePieceAction::ClearSelectionState_Implementation()
{
    // Clear hover state on currently hovered tile
    if (AGridTileBase* Hovered = CurrentHoveredTile.Get())
    {
        if (Tag_Default.IsValid())
        {
            Hovered->SendGameplayTag(Tag_Default);
        }
    }
}

int32 UConnectIt_PlacePieceAction::GetOwningControllerFactionID() const
{
    if (!IsValid(OwningController))
    {
        UE_LOG(LogTemp, Error,
            TEXT("PlacePieceAction: PostInitialiseAction -- "
                 "OwningController is null"));
        return -1;
    }

    int32 FactionId  = -1;
    if (OwningController->Implements<UGridFactionInterface>())
    {
        FactionId = IGridFactionInterface::Execute_GetFactionId(OwningController);
    }
    
    return FactionId;
}
