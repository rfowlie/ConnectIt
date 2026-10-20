// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "SkinTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SkinSubsystem.generated.h"

// Fires when the resolved skin ID for a category changes (player picked something, a
// level forced a skin, a saved selection was reloaded, ...). Consumers re-read the skin.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkinChanged, FGameplayTag, Category);

// Owns the player's skin selection and resolves it to a skin ID per category.
//
// GameInstance-level: it outlives level travel, exists on the main-menu level (where the
// options live), and is client-local -- cosmetics are never replicated, so it is not
// created on a dedicated server.
//
// The subsystem knows nothing about what a skin is. A skin is a row in a project-defined
// DataTable (USkinSettings::SkinTables); the subsystem resolves *which row* is active
// and hands the project a FDataTableRowHandle to read with its own row struct.
//
// Resolution precedence per category:
//     level-forced > player override > player preset > project default preset
// A candidate only counts if its ID exists as a row in that category's table; otherwise
// resolution falls through to the next level.
UCLASS()
class UNREALSKINMECHANICS_API USkinSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintAssignable, Category = "Skin")
	FOnSkinChanged OnSkinChanged;

	// --- Queries ---

	// Categories the player can customise (the keys of USkinSettings::SkinTables).
	UFUNCTION(BlueprintPure, Category = "Skin")
	TArray<FGameplayTag> GetCategories() const;

	// Every skin ID (row name) available for a category, for an options menu.
	UFUNCTION(BlueprintPure, Category = "Skin")
	TArray<FName> GetAvailableSkinIds(FGameplayTag Category) const;

	UFUNCTION(BlueprintPure, Category = "Skin")
	TArray<FName> GetAvailablePresetIds() const;

	// The active skin ID for a category after applying precedence; NAME_None if nothing
	// resolves (a warning is logged once per category).
	UFUNCTION(BlueprintPure, Category = "Skin")
	FName GetSkinId(FGameplayTag Category) const;

	// The active skin ID plus which precedence level produced it (for diagnostics / debug UI).
	UFUNCTION(BlueprintPure, Category = "Skin")
	FSkinResolution GetSkinResolution(FGameplayTag Category) const;

	// Human-readable configuration problems (missing/empty tables, bad preset table, presets
	// naming unknown categories or skin IDs, categories that resolve to nothing). Empty when
	// the setup looks correct.
	UFUNCTION(BlueprintPure, Category = "Skin")
	TArray<FString> GetConfigurationIssues() const;

	// The active skin's row -- table + row name -- for the project to read with its own
	// row struct. Null handle if nothing resolves.
	UFUNCTION(BlueprintPure, Category = "Skin")
	FDataTableRowHandle GetSkinRowHandle(FGameplayTag Category) const;

	// C++ convenience: the active skin's row as the project's row struct, or nullptr.
	template <typename RowType>
	const RowType* FindSkinRow(const FGameplayTag Category) const
	{
		return GetSkinRowHandle(Category).GetRow<RowType>(TEXT("USkinSubsystem::FindSkinRow"));
	}

	// The player's chosen preset ID (defaults to the project default preset).
	UFUNCTION(BlueprintPure, Category = "Skin")
	FName GetPresetId() const { return PresetId; }

	// The player's override for a category, or NAME_None if they haven't overridden it.
	UFUNCTION(BlueprintPure, Category = "Skin")
	FName GetOverride(FGameplayTag Category) const { return Overrides.FindRef(Category); }

	// --- Player selection (applies immediately; call SaveSelection to persist) ---

	UFUNCTION(BlueprintCallable, Category = "Skin")
	void SetPreset(FName NewPresetId);

	UFUNCTION(BlueprintCallable, Category = "Skin")
	void SetOverride(FGameplayTag Category, FName SkinId);

	UFUNCTION(BlueprintCallable, Category = "Skin")
	void ClearOverride(FGameplayTag Category);

	// --- Level-forced skins (runtime only, never saved) ---
	// Sit above player choices in precedence. The project calls these from its own level
	// config; the plugin knows nothing about levels.

	UFUNCTION(BlueprintCallable, Category = "Skin")
	void SetLevelForced(const TMap<FGameplayTag, FName>& ForcedSkinIds);

	UFUNCTION(BlueprintCallable, Category = "Skin")
	void ClearLevelForced();

	// --- Persistence ---

	// Writes the preset + overrides to the save slot (level-forced skins are never saved).
	UFUNCTION(BlueprintCallable, Category = "Skin")
	bool SaveSelection();

	// Re-reads the save slot, discarding unsaved changes -- the "Revert" of an
	// Apply/Revert options menu. Broadcasts OnSkinChanged for any category that changed.
	UFUNCTION(BlueprintCallable, Category = "Skin")
	void ReloadSelection();

private:

	void LoadTables();
	void LoadSelectionFromSlot();

	const FSkinPresetRowBase* FindPresetRow(FName Id) const;
	bool SkinIdExists(FGameplayTag Category, FName SkinId) const;
	FSkinResolution Resolve(FGameplayTag Category) const;
	FName ResolveSkinId(const FGameplayTag Category) const { return Resolve(Category).SkinId; }

	TMap<FGameplayTag, FName> SnapshotResolved() const;
	void BroadcastChangesSince(const TMap<FGameplayTag, FName>& Before);

	// Loaded once in Initialize so the read API can stay const.
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UDataTable>> SkinTables;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> PresetTable;

	FName PresetId;
	TMap<FGameplayTag, FName> Overrides;
	TMap<FGameplayTag, FName> LevelForced;

	// Categories already warned about, so an unresolved category logs once, not per read.
	mutable TSet<FGameplayTag> WarnedCategories;
};
