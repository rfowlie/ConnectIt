// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action/TurnBasedAction.h"
#include "ConnectIt_PlacePieceAction.generated.h"


class UConnectIt_BoardStateComponent;
class AConnectIt_GridPiece;
class AConnectIt_BoardManager;

// ConnectIt concrete action -- places a piece on the board
// Responsibilities:
//   - Validates hover and selection (empty, active tile)
//   - Sends GameplayTags to tiles for visual hover feedback
//   - Builds FTurnActionRequest and fires OnChangeRequested
//   - Has NO knowledge of pools, piece actors, or board state mutation
//   - Has NO visual responsibilities beyond sending tags to tiles
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CONNECTIT_API UConnectIt_PlacePieceAction : public UTurnBasedAction
{
    GENERATED_BODY()

public:

    UConnectIt_PlacePieceAction();

    // GameplayTags sent to tiles to drive their visual state
    // Set in editor -- tile Blueprint responds to these tags
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Tags")
    FGameplayTag Tag_ValidHover;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Tags")
    FGameplayTag Tag_InvalidHover;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Tags")
    FGameplayTag Tag_Default;

    // Request type tag sent to board manager
    // Must match ConnectItRequestTags::PlacePiece in board manager
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Tags")
    FGameplayTag Tag_RequestType;

protected:

    // base internal
    virtual void Activate_Internal_Implementation() override;

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

    // Gets owning faction ID from participant component slot index
    int32 GetOwningFactionID() const;
};