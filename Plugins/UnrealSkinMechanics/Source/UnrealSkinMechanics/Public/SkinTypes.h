// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "SkinTypes.generated.h"

// The ONLY row struct this plugin defines.
//
// The plugin deliberately knows nothing about what a skin *is* -- skin table rows are
// whatever struct the game project defines (meshes, materials, icons, per-faction data,
// UI tokens, ...). It only ever uses a skin row's NAME as the stable skin ID and hands
// the row back to the project (see USkinSubsystem::GetSkinRowHandle).
//
// A preset is different: the plugin has to read it to resolve a player's selection, so
// it needs the one field below. Projects derive from this to add their own display
// fields (name, icon, ...); the ROW NAME is the preset's stable ID.
USTRUCT(BlueprintType)
struct UNREALSKINMECHANICS_API FSkinPresetRowBase : public FTableRowBase
{
	GENERATED_BODY()

	// Skin category -> skin ID (a row name in that category's skin table). A category
	// with no entry falls back to the project's default preset.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skin")
	TMap<FGameplayTag, FName> SkinIds;
};

// Which precedence level produced a category's resolved skin ID.
UENUM(BlueprintType)
enum class ESkinSource : uint8
{
	None,			// nothing resolved
	LevelForced,
	PlayerOverride,
	PlayerPreset,
	DefaultPreset
};

// A category's resolved skin ID and where it came from (for diagnostics / debug UI).
USTRUCT(BlueprintType)
struct UNREALSKINMECHANICS_API FSkinResolution
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Skin")
	FName SkinId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Skin")
	ESkinSource Source = ESkinSource::None;
};
