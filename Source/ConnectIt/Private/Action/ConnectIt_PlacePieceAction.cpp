// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ConnectIt_PlacePieceAction.h"
#include "Board/Operations/ConnectIt_BoardOperations.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "ConnectIt_GameplayTags.h"
#include "ConnectIt_Structs.h"
#include "Interface/GridFactionInterface.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridTileRegistryBase.h"
#include "Tile/GridTileRegistryComponent.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"


bool UConnectIt_PlacePieceAction::ProducesRequestType_Implementation(FGameplayTag RequestType) const
{
    return RequestType == ConnectIt_Game_PlacePiece;
}

UConnectIt_PlacePieceAction::UConnectIt_PlacePieceAction()
{
    // Uses, per-turn cap and turn-end contribution live in the loadout's
    // PermanentActions entry and TurnEndRequirements tree, not here.

    // Cancellable -- shard or power activation cancels this,
    // and it reactivates after the optional action completes
    bIsCancellable = true;

    bRequiresSelection = true;
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
    if (!IsValid(Tile)) return false;

    const UConnectIt_BoardStateComponent* BoardState =
        UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    if (!IsValid(BoardState)) return false;

    // Position is not stored on the tile itself -- resolved via the
    // owning controller's tile registry
    const auto TileRegistry = UConnectIt_GameUtilityLibrary::GetTileRegistry(GetPlayerController());
    const FGridPosition Position = TileRegistry->GetPositionOfTile(Tile);

    // The same check the server makes of the request (and the AI of its
    // candidate moves)
    return FConnectItBoardOperation_PlacePiece::IsTilePlaceableAt(
        BoardState->GetCurrentState(), Position);   
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
    if (!TagActionMouseHover.IsValid()) return;

    Tile->SendGameplayTag(TagActionMouseHover);
}

void UConnectIt_PlacePieceAction::HandleHoverCleared_Implementation(AGridTileBase* PreviousTile)
{
    if (!IsValid(PreviousTile)) return;
    if (!TagActionGridState.IsValid()) return;

    PreviousTile->SendGameplayTag(TagActionGridState);
}

void UConnectIt_PlacePieceAction::HandleValidSelection_Implementation(AGridTileBase* Tile)
{
    // if (!IsValid(TileRegistry)) return;
    if (!IsValid(Tile)) return;

    const auto TileRegistry = UConnectIt_GameUtilityLibrary::GetTileRegistry(GetPlayerController());
    if (!TileRegistry)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleValidSelection_Implementation - Tire Registry nullptr"));
        return;
    }

    FGridPosition Position = TileRegistry->GetPositionOfTile(Tile);
    
    // Build the request -- board manager handles all mutation
    // Action has no knowledge of pools, piece actors, or state changes
    // The request's payload is the board change itself (see
    // FConnectItBoardOperation)
    FConnectItBoardOperation_PlacePiece Operation;
    Operation.Faction = GetOwningControllerFactionID();
    Operation.Position = Position;

    FTurnActionRequest Request;
    Request.RequestType = Operation.GetRequestType();
    Request.FactionID = Operation.Faction;
    Request.Payload = FInstancedStruct::Make(Operation);

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
        if (TagActionGridState.IsValid())
        {
            Hovered->SendGameplayTag(TagActionGridState);
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
