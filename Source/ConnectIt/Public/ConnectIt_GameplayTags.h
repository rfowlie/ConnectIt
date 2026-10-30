// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"


// --- Player actions (what a participant does on its turn; an action's tag) ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_PlayerAction_PlacePiece);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_PlayerAction_SwapPiece);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_PlayerAction_Shift);

// --- Board operations (a change to the board; an operation's tag) ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_PlacePiece);

// OperationTag tags for actions added alongside the generic UStackedState/
// UStackedStateMachine port -- see Docs (Action classes) for which action
// sends which
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_DestroyTileMultiplier);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_RemovePiece);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_SwapPieces);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_ForcePlacePiece);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_ToggleTileActive);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_SetTileLocked);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_SetChainPartner);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_MovePiece);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_AdjustMultiplier);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_TurnStart);
// The Chained tile's power (FConnectItTilePowerEffect_Chained): locked while its partner has no piece
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_TilePower_Chained);
// Push (FConnectItTilePowerEffect_Push): pushes a neighbouring piece one tile away
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_TilePower_Push);
// Vacuum (FConnectItTilePowerEffect_Vacuum): pulls the nearest piece in some direction onto its empty tile
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_TilePower_Vacuum);
// Repel (FConnectItTilePowerEffect_Repel): on a countdown, moves its piece to an open neighbour
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_TilePower_Repel);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_CapturePiece);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_Shift);
// Level only (FConnectItBoardOperation_ChangeGoals): no action produces it
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_ChangeGoals);
// Level only for now: tiles leaving / joining the board, and a whole new layout
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_RemoveTiles);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_AddTiles);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_ChangeLayout);
// Level only (FConnectItBoardOperation_BeginPhase)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_BoardOperation_BeginPhase);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Tile_ValidHover);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Tile_InvalidHover);
// The hover category of an Adventure hub map node (its grid hover component)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Adventure_MapNode);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Tile_Occupied);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Tile_Default);

// Board event tags -- keys into UGameEventTaskSubsystem's tag-keyed
// UGameEventTaskManager registry, used to gate visual sequencing so piece
// placement, scoring, and win visuals run fully in order instead of at once
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_PiecePlaced);
// Fired once when the board is initialised from the level -- react to it to
// create visuals for any designer-placed starting pieces
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_BoardSeeded);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_Scored);
// A tile power fired (see FConnectItBoardEvent_TilePowerTriggered)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_TilePowerTriggered);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_PlayerWin);
// A faction completed a win condition group that was not its last (see
// FConnectItBoardEvent_WinCheckpointReached)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_WinCheckpointReached);
// A faction met one of its lose conditions (see FConnectItBoardEvent_LoseConditionMet)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_LoseConditionMet);
// A side's goals were replaced mid-match (see FConnectItBoardEvent_GoalsChanged)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_GoalsChanged);
// Tiles left / joined the board, and the board changed to a new layout (see
// FConnectItBoardEvent_TilesRemoved / _TilesAdded / _LayoutChanged)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_TilesRemoved);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_TilesAdded);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_LayoutChanged);
// A phased level moved to a new phase (see FConnectItBoardEvent_PhaseChanged)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_PhaseChanged);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_TurnEnd);
// Server only, not a board event: queued right after a board change's events,
// so it completes once they have all played out -- the end of that change
// (see AConnectIt_GameMode::IsBoardChangeResolving)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_BoardChangeComplete);

// Board event tags for the new mutation types -- ForcePlacePiece
// deliberately reuses ConnectIt_Event_PiecePlaced instead of getting its
// own (same visual outcome: a piece appears on a tile)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_TileMultiplierDestroyed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_PieceRemoved);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_PiecesSwapped);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_TileActiveToggled);
// A tile was locked or unlocked (see FConnectItBoardEvent_TileLockChanged)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_TileLockChanged);
// A Chained tile's partner was set or cleared (see FConnectItBoardEvent_ChainPartnerChanged)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_ChainPartnerChanged);
// A piece moved to another tile, keeping its owner (see FConnectItBoardEvent_PieceMoved)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_PieceMoved);
// A tile's multiplier changed by an amount (see FConnectItBoardEvent_MultiplierChanged)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_MultiplierChanged);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_PieceCaptured);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_BoardShifted);
