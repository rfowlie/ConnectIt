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
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_CapturePiece);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Tile_ValidHover);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Tile_InvalidHover);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Tile_Occupied);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Tile_Default);

// Board event tags -- keys into UGameEventTaskSubsystem's tag-keyed
// UGameEventTaskManager registry, used to gate visual sequencing so piece
// placement, scoring, and win visuals run fully in order instead of at once
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_PiecePlaced);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_LineScored);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_PlayerWin);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_TurnEnd);

// Board event tags for the new mutation types -- ForcePlacePiece
// deliberately reuses ConnectIt_Event_PiecePlaced instead of getting its
// own (same visual outcome: a piece appears on a tile)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_TileMultiplierDestroyed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_PieceRemoved);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_PiecesSwapped);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_TileActiveToggled);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Event_PieceCaptured);
