// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConnectIt_GameUtilityLibrary.generated.h"

class AConnectIt_PlayerState;
struct FGridPosition;
class AGridTileBase;
class AConnectIt_GameState;
class UConnectIt_BoardStateComponent;
class UConnectIt_BoardManagerComponent;
class UConnectIt_BlackboardSubsystem;
class UTurnBasedParticipantManagerComponent;
class UGridHoverSubsystem;
class UGridTileRegistryBase;
class UGridPieceRegistryBase;
class UActionLoadoutDataAsset;
class UConnectIt_AIProfile;
class AConnectIt_GridPiece;
struct FConnectItRuleSet;

UCLASS()
class CONNECTIT_API UConnectIt_GameUtilityLibrary
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    // --- Board State ---

    // Returns the board state component (lives on AConnectIt_GameState)
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static UConnectIt_BoardStateComponent* GetBoardStateComponent(
        const UObject* WorldContextObject);

    // Returns the current world's TileRegistry, via
    // UConnectIt_BoardRegistrySubsystem -- one canonical instance per world,
    // resolved identically on server and every client.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static UGridTileRegistryBase* GetTileRegistry(
        const UObject* WorldContextObject);

    // Returns the current world's PieceRegistry, via
    // UConnectIt_BoardRegistrySubsystem. See GetTileRegistry.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static UGridPieceRegistryBase* GetPieceRegistry(
        const UObject* WorldContextObject);


    // --- Game Board ---
    // Read-only queries over the replicated board state
    // Replaces UConnectIt_GameFacade for the networked game -- one
    // static, stateless place for UI/AI to query the board instead of
    // an object bound to the old non-networked state machine

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
    meta = (WorldContext = "WorldContextObject"))
    static bool GetGridPositionForTile(
        const UObject* WorldContextObject,
        const AGridTileBase* Tile,
        FGridPosition& OutPosition);

    // Returns every tile registered on the board
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility|Board",
        meta = (WorldContext = "WorldContextObject"))
    static TArray<AGridTileBase*> GetAllGridTiles(
        const UObject* WorldContextObject);

    // Returns tiles that are active and unoccupied
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility|Board",
        meta = (WorldContext = "WorldContextObject"))
    static TArray<AGridTileBase*> GetEmptyGridTiles(
        const UObject* WorldContextObject);

    // Returns true if no faction piece occupies the tile
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility|Board",
        meta = (WorldContext = "WorldContextObject"))
    static bool IsTileEmpty(
        const UObject* WorldContextObject,
        const AGridTileBase* Tile);

    // Returns all tiles currently holding a piece belonging to FactionSlot
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility|Board",
        meta = (WorldContext = "WorldContextObject"))
    static TArray<AGridTileBase*> GetGridTilesWithFactionPieces(
        const UObject* WorldContextObject,
        int32 FactionSlot);

    // Returns the tile registered at a grid position -- null if none
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility|Board",
        meta = (WorldContext = "WorldContextObject"))
    static AGridTileBase* GetTileAtPosition(
        const UObject* WorldContextObject,
        FGridPosition Position);

    // Returns a random tile that is active and unoccupied -- null if
    // the board is full. Used by AI/utility fallback move selection
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility|Board",
        meta = (WorldContext = "WorldContextObject"))
    static AGridTileBase* GetRandomEmptyGridTile(
        const UObject* WorldContextObject);

    // Returns true once no tile remains valid for placement
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility|Board",
        meta = (WorldContext = "WorldContextObject"))
    static bool IsGameBoardFull(
        const UObject* WorldContextObject);

    // Returns true if the board has ended with FactionSlot as winner
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility|Board",
        meta = (WorldContext = "WorldContextObject"))
    static bool HasFactionWon(
        const UObject* WorldContextObject,
        int32 FactionSlot);


    // --- Match setup (live) ---
    // What this match is actually using. Never read the level config asset
    // for these -- it is only the starting template, and the server's
    // GameMode may have changed them (menu choices, rules that change
    // mid-level, a player's actions changing). These work on every machine.

    // The match's current rules (the GameState's replicated copy). False if
    // the GameState isn't available yet.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Match",
        meta = (WorldContext = "WorldContextObject"))
    static bool GetMatchRules(const UObject* WorldContextObject, FConnectItRuleSet& OutRules);

    // The AI opponent being played, or null (online match / not known yet)
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Match",
        meta = (WorldContext = "WorldContextObject"))
    static UConnectIt_AIProfile* GetOpponentProfile(const UObject* WorldContextObject);

    // The local player's current loadout (from their PlayerState), or null
    // until it has replicated
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Match",
        meta = (WorldContext = "WorldContextObject"))
    static UActionLoadoutDataAsset* GetLocalPlayerLoadout(const UObject* WorldContextObject);

    // --- Level setup (fixed) ---
    // Level-authored values that never change during a match, so every
    // machine reads them straight from the level's config.

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Level",
        meta = (WorldContext = "WorldContextObject"))
    static TSubclassOf<AConnectIt_GridPiece> GetPieceActorClass(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Level",
        meta = (WorldContext = "WorldContextObject"))
    static int32 GetPiecePoolInitialSize(const UObject* WorldContextObject);

    // --- Game State ---

    // Returns the ConnectIt game state
    // Use this instead of GetWorld()->GetGameState<AConnectIt_GameState>()
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static AConnectIt_GameState* GetConnectItGameState(
        const UObject* WorldContextObject);

    // retreive any players player state by index
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static void GetAllConnectItPlayerStates(
        TArray<AConnectIt_PlayerState*>& OutPlayerStates, const UObject* WorldContextObject);

    // shortcut to get the local players player state
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static AConnectIt_PlayerState* GetLocalConnectItPlayerState(
        const UObject* WorldContextObject);

    // Returns the player state for a given FactionID (== SlotIndex),
    // regardless of which machine calls it -- unlike
    // GetLocalConnectItPlayerState, which is scoped to the local player
    // only. Server-callable authority checks (e.g. SWAP's use-count gate in
    // UConnectIt_BoardRequestMediator) need this form.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static AConnectIt_PlayerState* GetPlayerStateForFaction(
        const UObject* WorldContextObject, int32 FactionID);

    // Returns the participant manager from game state
    // Shortcut for UI and systems that need turn phase or participant list
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static UTurnBasedParticipantManagerComponent* GetParticipantManager(
        const UObject* WorldContextObject);

    // Returns the current turn phase
    // Convenience for UI -- avoids two-step game state lookup
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static ETurnPhase GetCurrentTurnPhase(
        const UObject* WorldContextObject);

    // --- Local Player ---

    // Returns the slot index of the local player
    // Reads CachedSlotIndex from the participant component
    // on the first local player controller
    // Returns -1 if not found
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static int32 GetLocalPlayerSlotIndex(
        const UObject* WorldContextObject);

    // Returns true if it is currently the local player's turn
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static bool IsLocalPlayerTurn(
        const UObject* WorldContextObject);

    // --- Subsystems ---

    // Returns the ConnectIt blackboard subsystem
    // Use this in shard actions instead of raw GetSubsystem call
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static UConnectIt_BlackboardSubsystem* GetBlackboardSubsystem(
        const UObject* WorldContextObject);

    // Returns the grid hover subsystem
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static UGridHoverSubsystem* GetGridSubsystem(
        const UObject* WorldContextObject);
};