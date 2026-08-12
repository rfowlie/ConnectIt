// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_Pre);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_Post);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_SelectTile);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_PlacePiece);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConnectIt_Game_UpdateBoard);

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
