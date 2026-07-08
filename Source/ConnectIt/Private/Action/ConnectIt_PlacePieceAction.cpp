// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ConnectIt_PlacePieceAction.h"
#include "EngineUtils.h"
#include "Board/ConnectItBoardManager.h"
#include "Board/ConnectItBoardStateComponent.h"
#include "Tile/GridTileBase.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"


UConnectIt_PlacePieceAction::UConnectIt_PlacePieceAction()
{
    // Required -- turn cannot end without placing a piece
    bIsRequired        = true;

    // Cancellable -- shard or power activation cancels this
    // and it reactivates after the optional action completes
    bIsCancellable     = true;

    bRequiresSelection = true;
    MaxUsesPerTurn     = 1;
    CooldownTurns      = 0;
}

void UConnectIt_PlacePieceAction::OnActivated_Implementation()
{
    UE_LOG(LogTemp, Log,
        TEXT("PlacePieceAction: Activated — awaiting tile selection"));
}

void UConnectIt_PlacePieceAction::OnCancelled_Implementation()
{
    UE_LOG(LogTemp, Log,
        TEXT("PlacePieceAction: Cancelled — "
             "optional action taking priority"));
}

void UConnectIt_PlacePieceAction::OnCompleted_Implementation()
{
    UE_LOG(LogTemp, Log,
        TEXT("PlacePieceAction: Piece placed — action complete"));
}

bool UConnectIt_PlacePieceAction::IsValidHoverTile_Implementation(
    AGridTileBase* Tile) const
{
    // if (!IsValid(Tile)) return false;
    //
    // const UConnectItBoardStateComponent* BoardState =
    //     GetBoardStateComponent();
    // if (!IsValid(BoardState)) return false;
    //
    // // Tile must be valid for placement in current board state
    // // IsTileValidForPlacement checks both bIsActive and !IsOccupied
    // return BoardState->IsTileValidForPlacement(
    //     Tile->GetGridPosition());

    return false;
}

bool UConnectIt_PlacePieceAction::IsValidSelectionTile_Implementation(
    AGridTileBase* Tile) const
{
    // Selection validity matches hover validity
    return IsValidHoverTile_Implementation(Tile);
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
    // if (!IsValid(Tile)) return;
    //
    // // Build the request -- board manager handles all mutation
    // // Action has no knowledge of pools, piece actors, or state changes
    // FTurnActionRequest Request;
    // Request.RequestType = Tag_RequestType;
    // Request.Positions.Add(Tile->GetGridPosition());
    // Request.FactionID   = GetOwningFactionID();
    //
    // UE_LOG(LogTemp, Log,
    //     TEXT("PlacePieceAction: Selection confirmed at (%d,%d) "
    //          "faction %d — requesting board change"),
    //     Tile->GetGridPosition().X,
    //     Tile->GetGridPosition().Y,
    //     Request.FactionID);
    //
    // // Route to action component which sends to server
    // // Complete fires after request is sent -- not after server confirms
    // // Server confirmation comes via board state replication
    // RequestBoardChange(Request);
    // Complete();
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

const UConnectItBoardStateComponent* UConnectIt_PlacePieceAction::GetBoardStateComponent() const
{
    const UWorld* World = GetWorld();
    if (!IsValid(World)) return nullptr;

    for (TActorIterator<AConnectItBoardManager> It(World); It; ++It)
    {
        return It->ConnectItBoardState;
    }

    return nullptr;
}

int32 UConnectIt_PlacePieceAction::GetOwningFactionID() const
{
    if (!IsValid(OwningController)) return -1;

    const UTurnBasedParticipantComponent* ParticipantComp =
        OwningController->FindComponentByClass<UTurnBasedParticipantComponent>();

    return IsValid(ParticipantComp)
        ? ParticipantComp->CachedSlotIndex
        : -1;
}
