// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Piece/GridPieceRegistryBase.h"
#include "ConnectIt_PieceRegistry.generated.h"

class UConnectIt_BoardStateComponent;
class AConnectIt_PieceBase;

// ConnectIt-specific piece registry -- see UConnectIt_TileRegistry's header
// comment for the full reasoning (identical here): the plugin's
// UGridPieceRegistryBase stays project-agnostic, so board-state access lives
// on this subclass instead, resolved on demand rather than injected.
UCLASS(Blueprintable)
class CONNECTIT_API UConnectIt_PieceRegistry : public UGridPieceRegistryBase
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    UConnectIt_BoardStateComponent* GetBoardState() const;
    
    UFUNCTION(BlueprintImplementableEvent, Category = "ConnectIt|Board")
    void HandleGameEventComplete(FGameplayTag EventTag);

    // The piece at Position, or null if there is none -- never creates one
    // (unlike Get Piece). An actor there that isn't a ConnectIt piece is
    // logged and returns null.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    AConnectIt_PieceBase* GetPieceAt(FGridPosition Position) const;

    // A Piece Moved event's bookkeeping: the piece actor at From is now the
    // one at To. Returns it (null if none), e.g. to slide it there. Call from
    // the Piece Moved handler, on every machine.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    AGridPieceBase* MovePieceActor(FGridPosition From, FGridPosition To);

    // --- The board visualizer's bookkeeping (AConnectIt_BoardVisualizer) ---

    // Piece is now the one at Position (replacing any mapped there); its
    // position is set to match. Every mapping here does that.
    void MapPiece(const FGridPosition& Position, AGridPieceBase* Piece);

    // Stops mapping the piece at Position and returns it (null if none) --
    // the actor stays as it is, for its leaving visuals
    AGridPieceBase* UnmapPiece(const FGridPosition& Position);

    // The pieces at A and B trade places (either may be empty)
    void SwapPieceActors(const FGridPosition& A, const FGridPosition& B);

    // A shift: the piece at each start position becomes the one at the
    // matching end position (a rotation: every one is read before any is
    // written)
    void ShiftPieceActors(const TArray<FGridPosition>& StartPositions, const TArray<FGridPosition>& EndPositions);
};
