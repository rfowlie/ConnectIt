// Fill out your copyright notice in the Description page of Project Settings.


#include "ConnectIt_GameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_Pre, "ConnectIt.Game.Pre", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_Post, "ConnectIt.Game.Pre", "");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_SelectTile, "ConnectIt.Game.State.SelectTile", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_PlacePiece, "ConnectIt.Game.State.PlacePiece", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_UpdateBoard, "ConnectIt.Game.State.UpdateBoard", "");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_DestroyTileMultiplier, "ConnectIt.Game.State.DestroyTileMultiplier", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_RemovePiece, "ConnectIt.Game.State.RemovePiece", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_SwapPieces, "ConnectIt.Game.State.SwapPieces", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_ForcePlacePiece, "ConnectIt.Game.State.ForcePlacePiece", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_ToggleTileActive, "ConnectIt.Game.State.ToggleTileActive", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_CapturePiece, "ConnectIt.Game.State.CapturePiece", "");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Tile_ValidHover, "ConnectIt.Tile.ValidHover", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Tile_InvalidHover, "ConnectIt.Tile.InvalidHover", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Tile_Occupied, "ConnectIt.Tile.Occupied", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Tile_Default, "ConnectIt.Tile.Default", "");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_PiecePlaced, "ConnectIt.Event.PiecePlaced", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_LineScored, "ConnectIt.Event.LineScored", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_PlayerWin, "ConnectIt.Event.PlayerWin", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_TurnEnd, "ConnectIt.Event.TurnEnd", "");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_TileMultiplierDestroyed, "ConnectIt.Event.TileMultiplierDestroyed", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_PieceRemoved, "ConnectIt.Event.PieceRemoved", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_PiecesSwapped, "ConnectIt.Event.PiecesSwapped", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_TileActiveToggled, "ConnectIt.Event.TileActiveToggled", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_PieceCaptured, "ConnectIt.Event.PieceCaptured", "");