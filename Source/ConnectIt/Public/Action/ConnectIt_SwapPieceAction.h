// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action/TurnBasedAction.h"
#include "GridMechanicsBaseStructs.h"
#include "ConnectIt_SwapPieceAction.generated.h"

class UGridTileRegistryBase;

// ConnectIt concrete action -- SWAP: trades one of the acting player's own
// pieces for one of the opponent's. bIsRequired = true, but paired with
// PlacePiece as an alternate (not an addition) via
// UConnectIt_TurnBasedActionsComponent's CanAutoEndTurn override -- either
// one completing ends the turn, not both.
//
// Needs two tiles, which UTurnBasedAction's selection hooks don't natively
// support (built around one CurrentHoveredTile / one HandleValidSelection
// call per pick). Layers a small first/second-pick state machine on top
// instead: the first valid selection is remembered (bHasFirstSelection/
// FirstPosition) and highlighted; the second fires the actual request.
// Purely a client-side UX aid -- UConnectIt_BoardRequestMediator::
// HandleSwapPiecesRequest independently re-validates everything (faction
// ownership, occupancy, remaining uses) server-side regardless of what this
// class allowed locally.
//
// Same division of responsibility as UConnectIt_PlacePieceAction: no
// knowledge of pools, piece actors, or board-state mutation -- only builds
// the request and fires it.
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CONNECTIT_API UConnectIt_SwapPieceAction : public UTurnBasedAction
{
    GENERATED_BODY()

    UConnectIt_SwapPieceAction();

public:

    // TODO: is this even needed? Wouldn't there just be some default state
    // unless we want a unique default state for this action, which we would
    // read when the action fires are set all tiles and pieces too. Then this would
    // ensure that we return to this unique state when removing hover state
    // return to default state for tiles and pieces
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Tags")
    FGameplayTag TagActionGridState;
    
    // GameplayTags sent to tiles to drive their visual state -- same
    // pattern as UConnectIt_PlacePieceAction, plus Tag_FirstSelected for
    // the two-step pick.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Tags")
    FGameplayTag TagActionMouseHover;

    // Sent to the tile chosen as the first (acting player's own) selection
    // while awaiting the second pick -- distinct from Tag_ValidHover so the
    // "already picked" tile stays visually marked even once hover moves on
    // to the second tile.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Tags")
    FGameplayTag TagFirstSelection;

protected:

    virtual void Deactivate_Internal_Implementation() override;
    
    // Resolves and caches TileRegistry -- same pattern as
    // UConnectIt_PlacePieceAction.
    virtual void PostInitialiseAction_Implementation() override;

    virtual bool IsValidHoverTile_Implementation(AGridTileBase* Tile) const override;
    virtual bool IsValidSelectionTile_Implementation(AGridTileBase* Tile) const override;
    virtual void HandleValidHover_Implementation(AGridTileBase* Tile) override;
    virtual void HandleHoverCleared_Implementation(AGridTileBase* PreviousTile) override;
    virtual void HandleValidSelection_Implementation(AGridTileBase* Tile) override;
    virtual void ClearSelectionState_Implementation() override;

private:

    UPROPERTY()
    TObjectPtr<UGridTileRegistryBase> TileRegistry = nullptr;

    // Two-step selection scratch state. Deliberately not a UPROPERTY/not
    // networked -- purely local UX state driving hover/selection feedback;
    // the server never reads it, it only sees the final
    // FConnectItRequestSwapPieces payload. Reset on both a successful
    // second pick and ClearSelectionState (covers a server rejection via
    // the normal awaiting-confirmation reactivation).
    bool bHasSelectionFirst = false;
    FGridPosition PositionFirst;

    // bool bHasSelectionSecond = false;
    // FGridPosition PositionSecond;

    // Gets owning faction ID from participant component slot index --
    // same pattern as UConnectIt_PlacePieceAction.
    int32 GetOwningControllerFactionID() const;
    
    bool ResetAction();
};
