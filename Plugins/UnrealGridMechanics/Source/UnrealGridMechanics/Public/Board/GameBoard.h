// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "UObject/Interface.h"
#include "GameBoard.generated.h"

class AGridTileBase;
class AGridPieceBase;

// Singlecast mirrors of the multicast delegates a GameBoard exposes --
// UE interfaces can mandate functions but not UPROPERTY delegate members,
// so "provide this delegate" becomes "provide a Bind function taking a
// singlecast delegate of the matching signature." This is the real
// mechanism (what Blueprint's own "Bind Event to X" nodes compile down
// to): a generated singlecast dynamic delegate converts to FScriptDelegate,
// and a multicast delegate accepts .Add() of one -- so an implementer's
// _Implementation just does SourceObject->OnSomething.Add(Handler). Gives
// Blueprint a normal delegate pin (supports "Create Event") reached
// through a function call instead of a property pin, with full
// compile-time signature checking.
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnGridPieceVisualEventHandler, AGridPieceBase*, Piece);
DECLARE_DYNAMIC_DELEGATE(FOnBoardStateChangedHandler);

// "What do we need from a BoardManager/GameBoard" -- answers that
// question as a native interface instead of a class with components,
// so any object (not necessarily one shaped like ABoardManagerBase) can
// provide these capabilities. Grid-mechanics-generic tier only (tile/
// piece/piece-visual/generic-state) -- deliberately cannot cover
// scoring/win-condition/lifecycle, since those are typed against
// ConnectIt-only structs this plugin has no module dependency on. See
// IConnectItGameBoard (ConnectIt module) for that tier, which derives
// from this one.
//
// Functions are grouped by a Group_ prefix (Tile_/Piece_/PieceVisual_)
// plus matching Category metadata -- a single flat interface at this size
// is fine, but unprefixed names risk collision once a project-specific
// tier adds its own (e.g. SpawnPiece on the visual interpreter vs.
// SpawnPieceAt on the registry are already one bad search away from being
// confused).
//
// Every accessor returns by value, never by reference -- BlueprintNativeEvent's
// generated Execute_* wrapper supports the ProcessEvent/FFrame path for a
// hypothetical Blueprint override, and reference *return* types aren't
// something to trust there without verifying UHT actually accepts them.
// By-ref *parameters* are unaffected -- proven fine elsewhere in this
// codebase already (IConnectIt_ScoringRule::ApplyScoring does it today).
//
// Deliberately not implemented anywhere yet -- see the plan history for
// why (no real consumer today to validate the contract's shape against).
UINTERFACE(Blueprintable, BlueprintType)
class UNREALGRIDMECHANICS_API UGameBoard : public UInterface
{
    GENERATED_BODY()
};

class UNREALGRIDMECHANICS_API IGameBoard
{
    GENERATED_BODY()

public:

    // --- Tile (representative slice -- UGridTileRegistryBase has more
    // queries than this; every remaining one gets the same Tile_-prefixed
    // treatment: GridPositionToWorld/WorldToGridPosition/GetPositionOfTile/
    // GetRow/GetColumn/GetMinRow/GetMaxRow/GetMinColumn/GetMaxColumn/
    // GetRowPositions/GetColumnPositions) ---

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Tile")
    AGridTileBase* Tile_GetTileAtPosition(FGridPosition Position) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Tile")
    TArray<FGridPosition> Tile_GetAllTilePositions() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Tile")
    int32 Tile_GetRowCount() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Tile")
    int32 Tile_GetColumnCount() const;

    // --- Piece (low level + high level, matching UGridPieceRegistryBase) ---

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Piece")
    AGridPieceBase* Piece_GetPiece(FGridPosition Position);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Piece")
    AGridPieceBase* Piece_RetrievePiece(TSubclassOf<AGridPieceBase> PieceClass);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Piece")
    void Piece_ActivatePieceAt(AGridPieceBase* Piece, AGridTileBase* Tile);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Piece")
    void Piece_DeactivatePiece(AGridPieceBase* Piece);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Piece")
    void Piece_ReleasePiece(AGridPieceBase* Piece);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Piece")
    AGridPieceBase* Piece_SpawnPieceAt(TSubclassOf<AGridPieceBase> PieceClass, AGridTileBase* Tile);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|Piece")
    void Piece_DespawnPieceAt(FGridPosition Position);

    // --- Piece visual (UGridPieceSpawnInterpreterBase's two delegates) ---

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|PieceVisual")
    void PieceVisual_BindOnPieceSpawned(const FOnGridPieceVisualEventHandler& Handler);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameBoard|PieceVisual")
    void PieceVisual_BindOnPieceDespawned(const FOnGridPieceVisualEventHandler& Handler);
};
