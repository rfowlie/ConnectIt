// Fill out your copyright notice in the Description page of Project Settings.


#include "ConnectIt_GameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_Pre, "ConnectIt.Game.Pre", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_Post, "ConnectIt.Game.Pre", "");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_SelectTile, "ConnectIt.Game.State.SelectTile", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_PlacePiece, "ConnectIt.Game.State.PlacePiece", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Game_UpdateBoard, "ConnectIt.Game.State.UpdateBoard", "");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Tile_ValidHover, "ConnectIt.Tile.ValidHover", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Tile_InvalidHover, "ConnectIt.Tile.InvalidHover", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Tile_Occupied, "ConnectIt.Tile.Occupied", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Tile_Default, "ConnectIt.Tile.Default", "");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_PiecePlaced, "ConnectIt.Event.PiecePlaced", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_LineScored, "ConnectIt.Event.LineScored", "");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConnectIt_Event_PlayerWin, "ConnectIt.Event.PlayerWin", "");