// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action/TurnBasedPlayerAction.h"
#include "ConnectIt_PlacePieceAction.generated.h"


class UTurnBasedParticipantComponent;
class UConnectIt_BoardStateComponent;
class AConnectIt_PieceBase;
class UGridTileRegistryBase;

// ConnectIt concrete action -- places a piece on the board
// Responsibilities:
//   - Validates hover and selection (empty, active tile)
//   - Shows hover through the tile's cursor state and its hover cursor
//   - Builds FTurnActionRequest and fires OnChangeRequested
//   - Has NO knowledge of pools, piece actors, or board state mutation
//   - Has NO visual responsibilities beyond its cursor and tile cursor states
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CONNECTIT_API UConnectIt_PlacePieceAction : public UTurnBasedPlayerAction
{
    GENERATED_BODY()

    UConnectIt_PlacePieceAction();

public:
    
    // The cursor state a placeable hovered tile gets (the tile shows it in
    // OnCursorStateChanged; it goes back to neutral when hover leaves).
    // Default Grid.CursorState.Hovered.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Tags")
    FGameplayTag TagActionMouseHover;

    // The cursor that follows a placeable hovered tile; none = no cursor (the
    // tile's cursor state alone shows the hover)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Cursor", meta = (MustImplement = "/Script/UnrealGridMechanics.GridCursor"))
    TSubclassOf<AActor> HoverCursorClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Tags")
    FGameplayTag TagActionMouseHoverInvalid;

    // TODO: what is this for?
    // Operation tag tag sent to board manager
    // Must match ConnectItRequestTags::PlacePiece in board manager
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Tags")
    FGameplayTag Tag_RequestType;

    // The server checks a request's ActionTag against this so a PlacePiece
    // budget can only be spent by a PlacePiece request
    virtual bool ProducesRequestType_Implementation(FGameplayTag OperationTag) const override;

    // Its player action tag (ConnectIt.PlayerAction.*)
    virtual FGameplayTag GetActionTag_Implementation() const override;

protected:

    // base internal
    virtual void Activate_Internal_Implementation() override;

    // Resolves and caches TileRegistry via UConnectIt_GameUtilityLibrary --
    // called once by UTurnBasedPlayerAction::InitialiseAction, well before
    // Activate. Eager, fail-loud dependency resolution instead of repeated
    // on-demand lookups at point of use.
    virtual void PostInitialiseAction_Implementation() override;

    // turn internal
    virtual void OnCancelled_Implementation() override;
    virtual void OnCompleted_Implementation() override;
    virtual bool IsValidHoverTile_Implementation(AGridTileBase* Tile) const override;
    virtual bool IsValidSelectionTile_Implementation(AGridTileBase* Tile) const override;
    virtual void HandleValidHover_Implementation(AGridTileBase* Tile) override;
    virtual void HandleHoverCleared_Implementation(AGridTileBase* PreviousTile) override;
    virtual void HandleValidSelection_Implementation(AGridTileBase* Tile) override;
    virtual void ClearSelectionState_Implementation() override;

private:
    // TileRegistry lives on UConnectIt_BoardRegistrySubsystem now (one
    // canonical per-world instance) -- resolved once in
    // PostInitialiseAction_Implementation.
    // UPROPERTY()
    // TObjectPtr<UGridTileRegistryBase> TileRegistry = nullptr;

    // Gets owning faction ID from participant component slot index
    int32 GetOwningControllerFactionID() const;
};