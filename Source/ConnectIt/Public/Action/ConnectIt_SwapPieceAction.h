// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action/TurnBasedPlayerAction.h"
#include "GridMechanicsBaseStructs.h"
#include "ConnectIt_SwapPieceAction.generated.h"

class UGridTileRegistryBase;

// ConnectIt concrete action -- SWAP: trades one of the acting player's own
// pieces for one of the opponent's. Its match and turn uses and whether it
// ends the turn (alone, or as an alternative to PlacePiece) are configured in
// the loadout -- a PermanentActions entry and the TurnEndRequirements tree.
//
// Needs two tiles, which UTurnBasedPlayerAction's selection hooks don't natively
// support (built around one CurrentHoveredTile / one HandleValidSelection
// call per pick). Layers a small first/second-pick state machine on top
// instead: the first valid selection is remembered (bHasFirstSelection/
// FirstPosition) and marked (its cursor state and a marker cursor); the
// second fires the actual request.
// Purely a client-side UX aid -- UConnectIt_BoardRequestMediator::
// DispatchRequest (FConnectItBoardOperation_SwapPieces::CanApply) independently re-validates everything (faction
// ownership, occupancy, remaining uses) server-side regardless of what this
// class allowed locally.
//
// Same division of responsibility as UConnectIt_PlacePieceAction: no
// knowledge of pools, piece actors, or board-state mutation -- only builds
// the request and fires it.
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CONNECTIT_API UConnectIt_SwapPieceAction : public UTurnBasedPlayerAction
{
    GENERATED_BODY()

    UConnectIt_SwapPieceAction();

public:

    // The cursor state a valid hovered tile gets -- same as
    // UConnectIt_PlacePieceAction. Default Grid.CursorState.Hovered.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Tags")
    FGameplayTag TagActionMouseHover;

    // The cursor state of the tile chosen as the first (acting player's own)
    // pick while awaiting the second -- it keeps it while hover moves on.
    // Default Grid.CursorState.Selected.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Tags")
    FGameplayTag TagFirstSelection;

    // The follow cursor, by what's under it: one of the player's own pieces,
    // an opponent's piece, or a tile it can't pick. None = no cursor there.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Cursor", meta = (MustImplement = "/Script/UnrealGridMechanics.GridCursor"))
    TSubclassOf<AActor> OwnPieceCursorClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Cursor", meta = (MustImplement = "/Script/UnrealGridMechanics.GridCursor"))
    TSubclassOf<AActor> OpponentPieceCursorClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Cursor", meta = (MustImplement = "/Script/UnrealGridMechanics.GridCursor"))
    TSubclassOf<AActor> InvalidCursorClass;

    // Stays on the first pick until the second (or the action ends); none =
    // the first pick's cursor state alone marks it
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Cursor", meta = (MustImplement = "/Script/UnrealGridMechanics.GridCursor"))
    TSubclassOf<AActor> MarkerCursorClass;

    // Server-side check that a request's ActionTag matches its OperationTag
    virtual bool ProducesRequestType_Implementation(FGameplayTag OperationTag) const override;

    // Its player action tag (ConnectIt.PlayerAction.*)
    virtual FGameplayTag GetActionTag_Implementation() const override;

protected:

    virtual void Deactivate_Internal_Implementation() override;
    
    // Resolves and caches TileRegistry -- same pattern as
    // UConnectIt_PlacePieceAction.
    virtual void PostInitialiseAction_Implementation() override;

    virtual bool IsValidHoverTile_Implementation(AGridTileBase* Tile) const override;
    virtual bool IsValidSelectionTile_Implementation(AGridTileBase* Tile) const override;
    virtual void HandleValidHover_Implementation(AGridTileBase* Tile) override;
    virtual void HandleHoverCleared_Implementation(AGridTileBase* PreviousTile) override;
    virtual void HandleInvalidHover_Implementation(AGridTileBase* Tile) override;
    virtual void HandleValidSelection_Implementation(AGridTileBase* Tile) override;
    virtual void ClearSelectionState_Implementation() override;

private:

    UPROPERTY()
    TObjectPtr<UGridTileRegistryBase> TileRegistry = nullptr;

    // Two-step selection scratch state. Deliberately not a UPROPERTY/not
    // networked -- purely local UX state driving hover/selection feedback;
    // the server never reads it, it only sees the final
    // FConnectItBoardOperation_SwapPieces payload. Reset on both a successful
    // second pick and ClearSelectionState (covers a server rejection via
    // the normal awaiting-confirmation reactivation).
    bool bHasSelectionFirst = false;
    FGridPosition PositionFirst;

    // The first pick's marker (held from the action's cursors)
    UPROPERTY()
    TObjectPtr<AActor> MarkerCursor = nullptr;

    // The first-picked tile (null when there's no first pick)
    AGridTileBase* GetFirstTile() const;

    // The follow cursor's class for a valid hovered Tile: own or opponent's piece
    TSubclassOf<AActor> GetHoverCursorClassFor(AGridTileBase* Tile) const;

    // Unmarks the first pick (its state and marker) and forgets it
    void ClearFirstSelection();

    // bool bHasSelectionSecond = false;
    // FGridPosition PositionSecond;

    // Gets owning faction ID from participant component slot index --
    // same pattern as UConnectIt_PlacePieceAction.
    int32 GetOwningControllerFactionID() const;
    
    bool ResetAction();
};
