// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_BoardStateComponent.h"
#include "ConnectIt_Structs.h"
#include "GridMechanicsBaseStructs.h"
#include "TurnBasedMechanicsStructs.h"
#include "Board/BoardManagerBase.h"
#include "ConnectIt_GameBoard.generated.h"

class UConnectIt_PlacePieceGameEvent;
class UTurnBasedGameEvent;
class UConnectIt_ConfigComponent;
class UConnectIt_BoardRulesComponent;
class UConnectIt_BoardShiftComponent;
class UConnectIt_TileStateInterpreter;
class UConnectIt_ScoreInterpreter;

// Prototype: imitates AConnectIt_BoardManager's request-processing surface,
// but derives ABoardManagerBase, so its TileRegistry/PieceRegistry/
// PieceSpawnInterpreter come from that base as Instanced UObject properties
// (UGridTileRegistryBase/UGridPieceRegistryBase/UGridPieceSpawnInterpreterBase)
// instead of hardcoded ActorComponent slots -- see "Prototype
// ABoardManagerBase..." in the plan history for the underlying design and
// why that's a meaningful difference (true BP-only subclass swapping).
//
// Deliberately does NOT include an equivalent of UConnectIt_PieceSpawnInterpreter
// (the tag-reactive orchestrator that turns board-change tags into actual
// piece spawn/despawn calls). That class resolves its PieceRegistry/
// SpawnInterpreter references via GetOwner()->FindComponentByClass<T>(),
// which can never find a plain UObject -- it isn't attached to the Actor's
// component list. Porting that orchestrator to explicit injection (matching
// how UGridPieceRegistryBase::Initialise already works) is real, separate
// work, not done here. Until then, this board's PieceRegistry/
// PieceSpawnInterpreter are wired up and directly usable (e.g. from
// Blueprint or a debug console via GetPieceRegistry()/
// GetGridPieceSpawnInterpreter()), but nothing reactively drives them from
// board-state changes yet -- ProcessRequest below only ever commits board
// *state*, the same as it does on AConnectIt_BoardManager.
//
// Everything else here -- ProcessRequest and its eight HandleXRequest
// methods, BoardStateComponent/ConnectItConfigComponent/BoardRulesComponent/
// BoardShiftComponent/TileStateInterpreter/ScoreInterpreter -- is copied
// verbatim from AConnectIt_BoardManager: none of that logic ever touches
// TileRegistry/PieceRegistry directly, so none of it needed adapting for
// the new UObject types.
//
// Also fixes a known, pre-existing gap on the live class while at it:
// AConnectIt_BoardManager declares a BoardShiftComponent member but never
// actually constructs it (stays null unless a BP subclass adds one) --
// this class does construct it.
UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API AConnectIt_GameBoard : public ABoardManagerBase
{
    GENERATED_BODY()

public:

    AConnectIt_GameBoard();

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    UConnectIt_ConfigComponent* GetConfigComponent() const { return ConnectItConfigComponent; }

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    UConnectIt_BoardStateComponent* GetBoardStateComponent() const { return BoardStateComponent; }

    // Covariant override of ABoardManagerBase's slot -- callers holding an
    // AConnectIt_GameBoard* get the concrete type via GetBoardStateComponent()
    // above with zero cast; generic code holding only an ABoardManagerBase*
    // still gets a valid, correctly-typed pointer through this override.
    virtual UConnectIt_BoardStateComponent* GetBoardStateComponentBase() const override { return BoardStateComponent; }

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    UConnectIt_BoardShiftComponent* GetShiftStateComponent() const { return BoardShiftComponent; }

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    UConnectIt_BoardRulesComponent* GetBoardRulesComponent() const { return BoardRulesComponent; }

    // Called by GameMode after all tiles have registered
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    void InitialiseBoard(int32 NumFactions);

    // Entry point for all board change requests -- see
    // AConnectIt_BoardManager::ProcessRequest, identical contract.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    bool ProcessRequest(const FTurnActionRequest& Request);


protected:

    virtual void BeginPlay() override;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "ConnectIt|Components")
    TObjectPtr<UConnectIt_ConfigComponent> ConnectItConfigComponent = nullptr;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "ConnectIt|Components")
    TObjectPtr<UConnectIt_BoardStateComponent> BoardStateComponent = nullptr;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "ConnectIt|Components")
    TObjectPtr<UConnectIt_BoardShiftComponent> BoardShiftComponent = nullptr;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "ConnectIt|Components")
    TObjectPtr<UConnectIt_BoardRulesComponent> BoardRulesComponent = nullptr;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "ConnectIt|Interpreters")
    TObjectPtr<UConnectIt_TileStateInterpreter> TileStateInterpreter = nullptr;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "ConnectIt|Interpreters")
    TObjectPtr<UConnectIt_ScoreInterpreter> ScoreInterpreter = nullptr;

    // UFUNCTION(BlueprintReadWrite, Category = "ConnectIt|Game Board")
    TQueue<UTurnBasedGameEvent*> TurnBasedGameEventQueue;
    
    // read the board state and determine what events need to occur and in what order
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt|Game Board")
    void CreateGameEventsFromBoardUpdate();
    void ExecuteGameEvents();
    
    UPROPERTY(Instanced, EditDefaultsOnly, BlueprintReadOnly, Category = "ConnectIt|Game Board")
    UTurnBasedGameEvent* GameEventTest = nullptr;

    UPROPERTY(Instanced, EditDefaultsOnly, BlueprintReadOnly, Category = "ConnectIt|Game Board")
    UConnectIt_PlacePieceGameEvent* GameEventPlacePiece = nullptr;

private:

    void BindInterpreters();

    // --- Request Handlers ---
    // Copied verbatim from AConnectIt_BoardManager -- see that class for
    // the per-handler doc comments (unchanged here).
    bool HandlePlacePieceRequest(const FConnectItRequestPlacePiece& Request, int32 FactionID) const;
    bool HandleShiftRequest(const FConnectItRequestBoardShift& Request, int32 FactionID) const;
    bool HandleForcePlacePieceRequest(const FConnectItRequestForcePlacePiece& Request, int32 FactionID) const;
    bool HandleDestroyTileMultiplierRequest(const FConnectItRequestDestroyTileMultiplier& Request) const;
    bool HandleRemovePieceRequest(const FConnectItRequestRemovePiece& Request) const;
    bool HandleSwapPiecesRequest(const FConnectItRequestSwapPieces& Request) const;
    bool HandleToggleTileActiveRequest(const FConnectItRequestToggleTileActive& Request) const;
    bool HandleCapturePieceRequest(const FConnectItRequestCapturePiece& Request, int32 FactionID) const;
};
