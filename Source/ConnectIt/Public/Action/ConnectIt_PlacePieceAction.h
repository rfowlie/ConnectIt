// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action/TurnBasedAction.h"
#include "ConnectIt_PlacePieceAction.generated.h"


class AConnectIt_GridPiece;
class AConnectIt_BoardManager;

UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CONNECTIT_API UConnectIt_PlacePieceAction : public UTurnBasedAction
{
    GENERATED_BODY()

public:

    UConnectIt_PlacePieceAction();

protected:

    // --- UTurnBasedAction overrides ---

    virtual void OnActivated_Implementation() override;
    virtual void OnCancelled_Implementation() override;
    virtual void OnCompleted_Implementation() override;

    virtual bool IsValidHoverTile_Implementation(
        AGridTileBase* Tile) const override;

    virtual bool IsValidSelectionTile_Implementation(
        AGridTileBase* Tile) const override;

    virtual void HandleValidHover_Implementation(
        AGridTileBase* Tile) override;

    virtual void HandleHoverCleared_Implementation(
        AGridTileBase* PreviousTile) override;

    virtual void HandleValidSelection_Implementation(
        AGridTileBase* Tile) override;

    virtual void ClearVisuals_Implementation() override;

private:

    // Cached board actor reference — found once on activation
    UPROPERTY()
    TObjectPtr<AConnectIt_BoardManager> BoardManager = nullptr;

    // The piece placed this turn — returned to pool if action cancelled
    UPROPERTY()
    TObjectPtr<AConnectIt_GridPiece> PlacedPiece = nullptr;

    // Find and cache the board actor from the world
    AConnectIt_BoardManager* FindBoardActor() const;

    // Get the owning controller's faction ID via participant component
    int32 GetOwningFactionID() const;

    // Server RPC — validates and applies piece placement
    UFUNCTION(Server, Reliable)
    void ServerPlacePiece(AGridTileBase* TargetTile, int32 FactionID);
    void ServerPlacePiece_Implementation(AGridTileBase* TargetTile, int32 FactionID);

    // Tags used to communicate visual state to tiles
    // Defined as static members so Blueprint tiles can react to them
    static const FGameplayTag ConnectIt_Tile_ValidHover;
    static const FGameplayTag ConnectIt_Tile_InvalidHover;
    static const FGameplayTag ConnectIt_Tile_Occupied;
    static const FGameplayTag ConnectIt_Tile_Default;
};