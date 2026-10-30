// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

// The standard tile cursor states (AGridTileBase::SetCursorState). An action
// may set any tag -- these are the common ones. No tag = neutral.
UNREALGRIDMECHANICS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Grid_CursorState_Hovered);
UNREALGRIDMECHANICS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Grid_CursorState_Selected);
UNREALGRIDMECHANICS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Grid_CursorState_Valid);
UNREALGRIDMECHANICS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Grid_CursorState_Invalid);
