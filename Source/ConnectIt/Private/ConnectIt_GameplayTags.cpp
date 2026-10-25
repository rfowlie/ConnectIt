// Fill out your copyright notice in the Description page of Project Settings.


#include "ConnectIt_GameplayTags.h"

// --- Player actions (what a participant does on its turn; an action's tag) ---
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_PlayerAction_PlacePiece, "ConnectIt.PlayerAction.PlacePiece", "The Place Piece player action");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_PlayerAction_SwapPiece, "ConnectIt.PlayerAction.SwapPiece", "The Swap player action");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_PlayerAction_Shift, "ConnectIt.PlayerAction.Shift", "The Board Shift player action");

// --- Board operations (a change to the board; an operation's tag) ---


UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_PlacePiece, "ConnectIt.BoardOperation.PlacePiece", "");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_DestroyTileMultiplier, "ConnectIt.BoardOperation.DestroyTileMultiplier", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_RemovePiece, "ConnectIt.BoardOperation.RemovePiece", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_SwapPieces, "ConnectIt.BoardOperation.SwapPieces", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_ForcePlacePiece, "ConnectIt.BoardOperation.ForcePlacePiece", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_ToggleTileActive, "ConnectIt.BoardOperation.ToggleTileActive", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_SetTileLocked, "ConnectIt.BoardOperation.SetTileLocked", "Lock or unlock a tile (FConnectItBoardOperation_SetTileLocked)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_MovePiece, "ConnectIt.BoardOperation.MovePiece", "Move a piece to an empty tile, keeping its owner (FConnectItBoardOperation_MovePiece)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_AdjustMultiplier, "ConnectIt.BoardOperation.AdjustMultiplier", "Change a tile's multiplier by an amount (FConnectItBoardOperation_AdjustMultiplier)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_TurnStart, "ConnectIt.BoardOperation.TurnStart", "A turn started: the Turn Start tile powers that are ready fire (FConnectItBoardOperation_TurnStart)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_SetChainPartner, "ConnectIt.BoardOperation.SetChainPartner", "Set or clear a Chained tile's partner (FConnectItBoardOperation_SetChainPartner)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_TilePower_Push, "ConnectIt.TilePower.Push", "Push tile: pushes a neighbouring piece one tile away (FConnectItTilePowerEffect_Push)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_TilePower_Vacuum, "ConnectIt.TilePower.Vacuum", "Vacuum tile: pulls the nearest piece in some direction onto its empty tile (FConnectItTilePowerEffect_Vacuum)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_TilePower_Repel, "ConnectIt.TilePower.Repel", "Repel tile: on a countdown, moves its piece to an open neighbour (FConnectItTilePowerEffect_Repel)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_TilePower_Chained, "ConnectIt.TilePower.Chained", "Chained tile: locked while its partner has no piece (FConnectItTilePowerEffect_Chained, a Sync power)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_CapturePiece, "ConnectIt.BoardOperation.CapturePiece", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_Shift, "ConnectIt.BoardOperation.Shift", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_ChangeGoals, "ConnectIt.BoardOperation.ChangeGoals", "Level only: a side's goals replaced mid-match (FConnectItBoardOperation_ChangeGoals)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_RemoveTiles, "ConnectIt.BoardOperation.RemoveTiles", "Tiles taken off the board (FConnectItBoardOperation_RemoveTiles)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_AddTiles, "ConnectIt.BoardOperation.AddTiles", "Tiles put on the board (FConnectItBoardOperation_AddTiles)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_BeginPhase, "ConnectIt.BoardOperation.BeginPhase", "Level only: a phased level moves to a new phase (FConnectItBoardOperation_BeginPhase)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_BoardOperation_ChangeLayout, "ConnectIt.BoardOperation.ChangeLayout", "The board's tiles changed to a layout in one change (FConnectItBoardOperation_ChangeLayout)");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Tile_ValidHover, "ConnectIt.Tile.ValidHover", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Tile_InvalidHover, "ConnectIt.Tile.InvalidHover", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Tile_Occupied, "ConnectIt.Tile.Occupied", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Tile_Default, "ConnectIt.Tile.Default", "");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_PiecePlaced, "ConnectIt.Event.PiecePlaced", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_BoardSeeded, "ConnectIt.Event.BoardSeeded", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_Scored, "ConnectIt.Event.Scored", "Something scored as a result of a board change (see FConnectItBoardEvent_Scored)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_TilePowerTriggered, "ConnectIt.Event.TilePowerTriggered", "A tile power fired (see FConnectItBoardEvent_TilePowerTriggered)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_PlayerWin, "ConnectIt.Event.PlayerWin", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_LoseConditionMet, "ConnectIt.Event.LoseConditionMet", "A faction met one of its lose conditions, so it has lost (see FConnectItBoardEvent_LoseConditionMet)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_GoalsChanged, "ConnectIt.Event.GoalsChanged", "A side's goals were replaced mid-match (see FConnectItBoardEvent_GoalsChanged)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_TilesRemoved, "ConnectIt.Event.TilesRemoved", "Tiles left the board, with any pieces on them (see FConnectItBoardEvent_TilesRemoved)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_TilesAdded, "ConnectIt.Event.TilesAdded", "Tiles joined the board, with their starting pieces (see FConnectItBoardEvent_TilesAdded)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_PhaseChanged, "ConnectIt.Event.PhaseChanged", "A phased level moved to a new phase (see FConnectItBoardEvent_PhaseChanged)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_LayoutChanged, "ConnectIt.Event.LayoutChanged", "The board changed to a new layout -- after its tiles went and came (see FConnectItBoardEvent_LayoutChanged)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_WinCheckpointReached, "ConnectIt.Event.WinCheckpointReached", "A faction completed a win condition group that was not its last (see FConnectItBoardEvent_WinCheckpointReached)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_BoardChangeComplete, "ConnectIt.Event.BoardChangeComplete", "Server only: queued after a board change's events, so it completes once they have all played out");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_TurnEnd, "ConnectIt.Event.TurnEnd", "");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_TileMultiplierDestroyed, "ConnectIt.Event.TileMultiplierDestroyed", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_PieceRemoved, "ConnectIt.Event.PieceRemoved", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_PiecesSwapped, "ConnectIt.Event.PiecesSwapped", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_TileActiveToggled, "ConnectIt.Event.TileActiveToggled", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_TileLockChanged, "ConnectIt.Event.TileLockChanged", "A tile was locked or unlocked (see FConnectItBoardEvent_TileLockChanged)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_PieceMoved, "ConnectIt.Event.PieceMoved", "A piece moved to another tile, keeping its owner (see FConnectItBoardEvent_PieceMoved)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_MultiplierChanged, "ConnectIt.Event.MultiplierChanged", "A tile's multiplier changed by an amount (see FConnectItBoardEvent_MultiplierChanged)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_ChainPartnerChanged, "ConnectIt.Event.ChainPartnerChanged", "A Chained tile's partner was set or cleared (see FConnectItBoardEvent_ChainPartnerChanged)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_PieceCaptured, "ConnectIt.Event.PieceCaptured", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_BoardShifted, "ConnectIt.Event.BoardShifted", "");