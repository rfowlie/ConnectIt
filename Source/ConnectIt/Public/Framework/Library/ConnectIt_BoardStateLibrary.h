// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConnectIt_Structs.h"
#include "ConnectIt_BoardStateLibrary.generated.h"

// Blueprint-callable queries over FConnectItBoardState/FConnectItTileData,
// taken by const& rather than resolved from a component. USTRUCT member
// functions can't carry UFUNCTION (an engine/UHT restriction), so
// FConnectItBoardState's own inline helpers (GetTileData/IsTileOccupied/
// IsTileActive/GetScore/etc.) are unreachable from Blueprint as written --
// this library is that missing surface, not a new source of truth. Each
// function below is a one-line call into the struct's own method, so the
// inline C++ path stays hot and there is exactly one implementation of each
// rule.
//
// Deliberately NOT wrapped on UConnectIt_BoardStateComponent instead (that
// class's own header used to carry a commented-out draft of this exact
// idea) -- a component-bound wrapper only works for the component's own
// live CurrentState, but board-overlay UI also wants GetPreviousState()
// (to diff what just changed) and MinMax/AI code operates on detached,
// hypothetical FConnectItBoardState values that were never on a component
// at all. Taking the struct by value/const& serves all three the same way.
UCLASS()
class CONNECTIT_API UConnectIt_BoardStateLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    // --- Mirrors of FConnectItBoardState's inline helpers ---

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board State",
        meta = (DisplayName = "Get Tile Data (Board State)"))
    static bool GetTileData(
        const FConnectItBoardState& State,
        FGridPosition Position,
        FConnectItTileData& OutData);

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board State",
        meta = (DisplayName = "Is Tile Occupied (Board State)"))
    static bool IsTileOccupied(const FConnectItBoardState& State, FGridPosition Position);

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board State",
        meta = (DisplayName = "Is Tile Active (Board State)"))
    static bool IsTileActive(const FConnectItBoardState& State, FGridPosition Position);

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board State",
        meta = (DisplayName = "Is Tile Valid For Placement (Board State)"))
    static bool IsTileValidForPlacement(const FConnectItBoardState& State, FGridPosition Position);

    // -1 when the position is empty or isn't on this board
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board State")
    static int32 GetTileFaction(const FConnectItBoardState& State, FGridPosition Position);

    // 1.0 (the neutral default) when the position isn't on this board
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board State")
    static float GetTileMultiplier(const FConnectItBoardState& State, FGridPosition Position);

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board State",
        meta = (DisplayName = "Get Faction Score (Board State)"))
    static float GetFactionScore(const FConnectItBoardState& State, int32 FactionSlot);

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board State")
    static int32 GetNumTiles(const FConnectItBoardState& State);

    // --- Overlay convenience -- no C++ equivalent exists on the struct today ---

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board State")
    static TArray<FGridPosition> GetPositionsForFaction(
        const FConnectItBoardState& State, int32 FactionSlot);

    // What a multiplier overlay actually iterates -- every position whose
    // multiplier has been incremented above the given baseline.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board State")
    static TArray<FGridPosition> GetPositionsWithMultiplierAbove(
        const FConnectItBoardState& State, float BaseMultiplier = 1.f);

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board State")
    static TArray<FGridPosition> GetInactivePositions(const FConnectItBoardState& State);

    // Positions whose FConnectItTileData differs between two states -- pair
    // with GetPreviousState()/GetCurrentState() so an overlay can animate
    // exactly what moved without index-matching TilePositions/TileDataArray
    // by hand in Blueprint.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board State")
    static TArray<FGridPosition> GetChangedPositions(
        const FConnectItBoardState& Previous, const FConnectItBoardState& Current);
};
