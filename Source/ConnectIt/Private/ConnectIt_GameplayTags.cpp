// Fill out your copyright notice in the Description page of Project Settings.


#include "ConnectIt_GameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_Pre, "ConnectIt.Game.Pre", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_Post, "ConnectIt.Game.Post", "");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_SelectTile, "ConnectIt.Game.State.SelectTile", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_PlacePiece, "ConnectIt.Game.State.PlacePiece", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_UpdateBoard, "ConnectIt.Game.State.UpdateBoard", "");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_DestroyTileMultiplier, "ConnectIt.Game.State.DestroyTileMultiplier", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_RemovePiece, "ConnectIt.Game.State.RemovePiece", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_SwapPieces, "ConnectIt.Game.State.SwapPieces", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_ForcePlacePiece, "ConnectIt.Game.State.ForcePlacePiece", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_ToggleTileActive, "ConnectIt.Game.State.ToggleTileActive", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_CapturePiece, "ConnectIt.Game.State.CapturePiece", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_Shift, "ConnectIt.Game.State.Shift", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_ChangeGoals, "ConnectIt.Game.State.ChangeGoals", "Level only: a side's goals replaced mid-match (FConnectItBoardOperation_ChangeGoals)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_RemoveTiles, "ConnectIt.Game.State.RemoveTiles", "Tiles taken off the board (FConnectItBoardOperation_RemoveTiles)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_AddTiles, "ConnectIt.Game.State.AddTiles", "Tiles put on the board (FConnectItBoardOperation_AddTiles)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_BeginPhase, "ConnectIt.Game.State.BeginPhase", "Level only: a phased level moves to a new phase (FConnectItBoardOperation_BeginPhase)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_ChangeLayout, "ConnectIt.Game.State.ChangeLayout", "The board's tiles changed to a layout in one change (FConnectItBoardOperation_ChangeLayout)");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Tile_ValidHover, "ConnectIt.Tile.ValidHover", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Tile_InvalidHover, "ConnectIt.Tile.InvalidHover", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Tile_Occupied, "ConnectIt.Tile.Occupied", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Tile_Default, "ConnectIt.Tile.Default", "");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_PiecePlaced, "ConnectIt.Event.PiecePlaced", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_BoardSeeded, "ConnectIt.Event.BoardSeeded", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_Scored, "ConnectIt.Event.Scored", "Something scored as a result of a board change (see FConnectItBoardEvent_Scored)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_PowerTriggered, "ConnectIt.Event.PowerTriggered", "A tile power fired (see FConnectItBoardEvent_PowerTriggered)");
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
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_PieceCaptured, "ConnectIt.Event.PieceCaptured", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_BoardShifted, "ConnectIt.Event.BoardShifted", "");