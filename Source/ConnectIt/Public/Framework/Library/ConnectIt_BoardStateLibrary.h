// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConnectIt_Structs.h"
#include "StructUtils/InstancedStruct.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
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

    // --- For Blueprint AI strategies (and anything else that plans a move) ---

    // Every position a piece may be placed on right now -- the same check
    // the server makes of a Place Piece request
    // (FConnectItBoardOperation_PlacePiece::IsTilePlaceable).
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board State",
        meta = (DisplayName = "Get Placeable Positions (Board State)"))
    static TArray<FGridPosition> GetPlaceablePositions(const FConnectItBoardState& State);

    // "What would happen if Faction made this move?" -- without making it.
    // Operation is a board operation struct (e.g. Place Piece with a
    // Position, wrapped with Make Instanced Struct); Rules is the match's rule
    // set (e.g. the AI decision context's). Runs the very code the server
    // runs for a real request, on a copy: Board itself is never changed.
    //
    // True pin: the move is possible. ResultBoard is the board afterwards;
    // PointsScored what it scored (for whoever scored); NumScoredEvents how
    // many things scored (under the Lines rule: how many lines it completed);
    // Events everything that happened, in order (including a WinCheckpointReached
    // for each win group the move completes that isn't the faction's last).
    // ResultBoard's game-over / winner fields and GroupsCompleted are the rule
    // set's own verdict.
    //
    // False pin: Operation isn't a board operation, or can't be made on this
    // board (an occupied tile...). ResultBoard is then just Board.
    //
    // The operation's own Faction field is ignored: Faction is used instead,
    // as the server does with the requester's.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board State",
        meta = (ExpandBoolAsExecs = "ReturnValue"))
    static bool SimulateOperation(
        const FConnectItBoardState& Board,
        const FConnectItRuleSet& Rules,
        const FInstancedStruct& Operation,
        int32 Faction,
        FConnectItBoardState& ResultBoard,
        float& PointsScored,
        int32& NumScoredEvents,
        FConnectItBoardChangeEvent& Events);
};
