// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_Pre);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_Post);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_SelectTile);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_PlacePiece);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_UpdateBoard);

// RequestType tags for actions added alongside the generic UStackedState/
// UStackedStateMachine port -- see Docs (Action classes) for which action
// sends which
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_DestroyTileMultiplier);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_RemovePiece);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_SwapPieces);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_ForcePlacePiece);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_ToggleTileActive);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_SetTileLocked);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_CapturePiece);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_Shift);
// Level only (FConnectItBoardOperation_ChangeGoals): no action produces it
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_ChangeGoals);
// Level only for now: tiles leaving / joining the board, and a whole new layout
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_RemoveTiles);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_AddTiles);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_ChangeLayout);
// Level only (FConnectItBoardOperation_BeginPhase)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_BeginPhase);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Tile_ValidHover);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Tile_InvalidHover);
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
// A tile power fired (see FConnectItBoardEvent_PowerTriggered)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_PowerTriggered);
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
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_PieceCaptured);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_BoardShifted);
