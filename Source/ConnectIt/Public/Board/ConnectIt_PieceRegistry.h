// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Piece/GridPieceRegistryBase.h"
#include "ConnectIt_PieceRegistry.generated.h"

class UConnectIt_BoardStateComponent;

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

    // A Piece Moved event's bookkeeping: the piece actor at From is now the
    // one at To. Returns it (null if none), e.g. to slide it there. Call from
    // the Piece Moved handler, on every machine.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    AGridPieceBase* MovePieceActor(FGridPosition From, FGridPosition To);

protected:

    // TODO: figure out if these are useful
    void HandleBoardShifted();
    void HandleBoardPiecesSwapped();
    void HandleBoardPieceRemoved();
};
