// Copyright Epic Games, Inc. All Rights Reserved.

#include "Subsystem/SkinSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Save/SkinSaveGame.h"
#include "Settings/SkinSettings.h"
#include "SkinTypes.h"
#include "UnrealSkinMechanics.h"


bool USkinSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Cosmetics are client-local presentation; a dedicated server has nothing to skin.
	return !IsRunningDedicatedServer() && Super::ShouldCreateSubsystem(Outer);
}

void USkinSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadTables();
	LoadSelectionFromSlot();
}

void USkinSubsystem::LoadTables()
{
	const USkinSettings* Settings = USkinSettings::Get();

	SkinTables.Reset();
	for (const TPair<FGameplayTag, TSoftObjectPtr<UDataTable>>& Pair : Settings->SkinTables)
	{
		if (!Pair.Key.IsValid())
		{
			UE_LOG(LogSkinMechanics, Warning,
				TEXT("USkinSubsystem: SkinTables has an entry with an invalid category tag -- ignored"));
			continue;
		}

		UDataTable* Table = Pair.Value.LoadSynchronous();
		if (!IsValid(Table))
		{
			UE_LOG(LogSkinMechanics, Warning,
				TEXT("USkinSubsystem: skin table for category '%s' is unset or failed to load"),
				*Pair.Key.ToString());
			continue;
		}

		SkinTables.Add(Pair.Key, Table);
	}

	PresetTable = nullptr;
	if (!Settings->PresetTable.IsNull())
	{
		UDataTable* Table = Settings->PresetTable.LoadSynchronous();
		if (!IsValid(Table))
		{
			UE_LOG(LogSkinMechanics, Warning,
				TEXT("USkinSubsystem: PresetTable failed to load"));
		}
		else if (!Table->GetRowStruct() || !Table->GetRowStruct()->IsChildOf(FSkinPresetRowBase::StaticStruct()))
		{
			UE_LOG(LogSkinMechanics, Error,
				TEXT("USkinSubsystem: PresetTable '%s' row struct must derive from FSkinPresetRowBase -- presets disabled"),
				*Table->GetName());
		}
		else
		{
			PresetTable = Table;
		}
	}
}

void USkinSubsystem::LoadSelectionFromSlot()
{
	const USkinSettings* Settings = USkinSettings::Get();

	// No (or unreadable) save -> project defaults.
	PresetId = Settings->DefaultPresetId;
	Overrides.Reset();

	if (!UGameplayStatics::DoesSaveGameExist(Settings->SaveSlotName, Settings->SaveUserIndex))
	{
		return;
	}

	const USkinSaveGame* Save = Cast<USkinSaveGame>(
		UGameplayStatics::LoadGameFromSlot(Settings->SaveSlotName, Settings->SaveUserIndex));
	if (!IsValid(Save))
	{
		UE_LOG(LogSkinMechanics, Warning,
			TEXT("USkinSubsystem: save slot '%s' exists but is not a USkinSaveGame -- using defaults"),
			*Settings->SaveSlotName);
		return;
	}

	// A saved None preset is fine: resolution falls through to the default preset.
	PresetId = Save->PresetId;
	Overrides = Save->Overrides;
}

// --- Queries ---

TArray<FGameplayTag> USkinSubsystem::GetCategories() const
{
	TArray<FGameplayTag> Out;
	SkinTables.GetKeys(Out);
	return Out;
}

TArray<FName> USkinSubsystem::GetAvailableSkinIds(const FGameplayTag Category) const
{
	if (const TObjectPtr<UDataTable>* Table = SkinTables.Find(Category))
	{
		return (*Table)->GetRowNames();
	}
	return {};
}

TArray<FName> USkinSubsystem::GetAvailablePresetIds() const
{
	return IsValid(PresetTable) ? PresetTable->GetRowNames() : TArray<FName>();
}

FName USkinSubsystem::GetSkinId(const FGameplayTag Category) const
{
	return ResolveSkinId(Category);
}

FDataTableRowHandle USkinSubsystem::GetSkinRowHandle(const FGameplayTag Category) const
{
	FDataTableRowHandle Handle;

	const FName SkinId = ResolveSkinId(Category);
	if (SkinId.IsNone())
	{
		return Handle;
	}

	if (const TObjectPtr<UDataTable>* Table = SkinTables.Find(Category))
	{
		Handle.DataTable = *Table;
		Handle.RowName = SkinId;
	}
	return Handle;
}

// --- Resolution ---

const FSkinPresetRowBase* USkinSubsystem::FindPresetRow(const FName Id) const
{
	if (!IsValid(PresetTable) || Id.IsNone())
	{
		return nullptr;
	}

	// PresetTable's row struct was verified to derive from FSkinPresetRowBase in LoadTables,
	// and the base is the first (only) base of every derived row, so this cast is safe.
	return reinterpret_cast<const FSkinPresetRowBase*>(PresetTable->FindRowUnchecked(Id));
}

bool USkinSubsystem::SkinIdExists(const FGameplayTag Category, const FName SkinId) const
{
	if (SkinId.IsNone())
	{
		return false;
	}

	const TObjectPtr<UDataTable>* Table = SkinTables.Find(Category);
	return Table && (*Table)->GetRowMap().Contains(SkinId);
}

FSkinResolution USkinSubsystem::GetSkinResolution(const FGameplayTag Category) const
{
	return Resolve(Category);
}

FSkinResolution USkinSubsystem::Resolve(const FGameplayTag Category) const
{
	const auto Make = [](const FName Id, const ESkinSource Source)
	{
		FSkinResolution Result;
		Result.SkinId = Id;
		Result.Source = Source;
		return Result;
	};

	// level-forced > player override > player preset > project default preset
	if (const FName* Forced = LevelForced.Find(Category); Forced && SkinIdExists(Category, *Forced))
	{
		return Make(*Forced, ESkinSource::LevelForced);
	}

	if (const FName* Override = Overrides.Find(Category); Override && SkinIdExists(Category, *Override))
	{
		return Make(*Override, ESkinSource::PlayerOverride);
	}

	if (const FSkinPresetRowBase* Preset = FindPresetRow(PresetId))
	{
		if (const FName* Id = Preset->SkinIds.Find(Category); Id && SkinIdExists(Category, *Id))
		{
			return Make(*Id, ESkinSource::PlayerPreset);
		}
	}

	if (const FSkinPresetRowBase* Default = FindPresetRow(USkinSettings::Get()->DefaultPresetId))
	{
		if (const FName* Id = Default->SkinIds.Find(Category); Id && SkinIdExists(Category, *Id))
		{
			return Make(*Id, ESkinSource::DefaultPreset);
		}
	}

	if (SkinTables.Contains(Category) && !WarnedCategories.Contains(Category))
	{
		WarnedCategories.Add(Category);
		UE_LOG(LogSkinMechanics, Warning,
			TEXT("USkinSubsystem: no skin resolves for category '%s' (nothing forced, overridden, or "
				 "in the selected/default preset exists in its table)"),
			*Category.ToString());
	}
	return FSkinResolution();
}

TArray<FString> USkinSubsystem::GetConfigurationIssues() const
{
	TArray<FString> Issues;
	const USkinSettings* Settings = USkinSettings::Get();

	if (Settings->SkinTables.IsEmpty())
	{
		Issues.Add(TEXT("SkinTables is empty -- no categories are configured."));
	}

	for (const TPair<FGameplayTag, TSoftObjectPtr<UDataTable>>& Pair : Settings->SkinTables)
	{
		if (!Pair.Key.IsValid())
		{
			Issues.Add(TEXT("SkinTables has an entry with an invalid category tag."));
			continue;
		}

		const TObjectPtr<UDataTable>* Table = SkinTables.Find(Pair.Key);
		if (!Table || !IsValid(*Table))
		{
			Issues.Add(FString::Printf(TEXT("Category '%s': skin table is unset or failed to load."),
				*Pair.Key.ToString()));
		}
		else if ((*Table)->GetRowMap().IsEmpty())
		{
			Issues.Add(FString::Printf(TEXT("Category '%s': skin table '%s' has no rows."),
				*Pair.Key.ToString(), *(*Table)->GetName()));
		}
	}

	if (Settings->PresetTable.IsNull())
	{
		Issues.Add(TEXT("PresetTable is not set -- presets are disabled."));
	}
	else if (!IsValid(PresetTable))
	{
		Issues.Add(TEXT("PresetTable failed to load or its row struct does not derive from FSkinPresetRowBase."));
	}
	else
	{
		if (Settings->DefaultPresetId.IsNone())
		{
			Issues.Add(TEXT("DefaultPresetId is not set."));
		}
		else if (!FindPresetRow(Settings->DefaultPresetId))
		{
			Issues.Add(FString::Printf(TEXT("DefaultPresetId '%s' is not a row in the preset table."),
				*Settings->DefaultPresetId.ToString()));
		}

		for (const FName PresetRowName : PresetTable->GetRowNames())
		{
			const FSkinPresetRowBase* Preset = FindPresetRow(PresetRowName);
			if (!Preset)
			{
				continue;
			}

			for (const TPair<FGameplayTag, FName>& Entry : Preset->SkinIds)
			{
				if (!SkinTables.Contains(Entry.Key))
				{
					Issues.Add(FString::Printf(TEXT("Preset '%s': category '%s' has no skin table."),
						*PresetRowName.ToString(), *Entry.Key.ToString()));
				}
				else if (!SkinIdExists(Entry.Key, Entry.Value))
				{
					Issues.Add(FString::Printf(TEXT("Preset '%s': skin ID '%s' is not a row in the '%s' table."),
						*PresetRowName.ToString(), *Entry.Value.ToString(), *Entry.Key.ToString()));
				}
			}
		}
	}

	for (const TPair<FGameplayTag, TObjectPtr<UDataTable>>& Pair : SkinTables)
	{
		if (Resolve(Pair.Key).Source == ESkinSource::None)
		{
			Issues.Add(FString::Printf(TEXT("Category '%s' currently resolves to no skin."),
				*Pair.Key.ToString()));
		}
	}

	return Issues;
}

TMap<FGameplayTag, FName> USkinSubsystem::SnapshotResolved() const
{
	TMap<FGameplayTag, FName> Snapshot;
	for (const TPair<FGameplayTag, TObjectPtr<UDataTable>>& Pair : SkinTables)
	{
		Snapshot.Add(Pair.Key, ResolveSkinId(Pair.Key));
	}
	return Snapshot;
}

void USkinSubsystem::BroadcastChangesSince(const TMap<FGameplayTag, FName>& Before)
{
	// A warning already emitted for an unresolved category shouldn't suppress a later one
	// once the selection changes.
	WarnedCategories.Reset();

	for (const TPair<FGameplayTag, TObjectPtr<UDataTable>>& Pair : SkinTables)
	{
		const FName* Old = Before.Find(Pair.Key);
		if (!Old || *Old != ResolveSkinId(Pair.Key))
		{
			OnSkinChanged.Broadcast(Pair.Key);
		}
	}
}

// --- Player selection ---

void USkinSubsystem::SetPreset(const FName NewPresetId)
{
	const TMap<FGameplayTag, FName> Before = SnapshotResolved();
	PresetId = NewPresetId;
	BroadcastChangesSince(Before);
}

void USkinSubsystem::SetOverride(const FGameplayTag Category, const FName SkinId)
{
	const TMap<FGameplayTag, FName> Before = SnapshotResolved();
	Overrides.Add(Category, SkinId);
	BroadcastChangesSince(Before);
}

void USkinSubsystem::ClearOverride(const FGameplayTag Category)
{
	const TMap<FGameplayTag, FName> Before = SnapshotResolved();
	Overrides.Remove(Category);
	BroadcastChangesSince(Before);
}

// --- Level-forced ---

void USkinSubsystem::SetLevelForced(const TMap<FGameplayTag, FName>& ForcedSkinIds)
{
	const TMap<FGameplayTag, FName> Before = SnapshotResolved();
	LevelForced = ForcedSkinIds;
	BroadcastChangesSince(Before);
}

void USkinSubsystem::ClearLevelForced()
{
	const TMap<FGameplayTag, FName> Before = SnapshotResolved();
	LevelForced.Reset();
	BroadcastChangesSince(Before);
}

// --- Persistence ---

bool USkinSubsystem::SaveSelection()
{
	const USkinSettings* Settings = USkinSettings::Get();

	USkinSaveGame* Save = Cast<USkinSaveGame>(
		UGameplayStatics::CreateSaveGameObject(USkinSaveGame::StaticClass()));
	if (!IsValid(Save))
	{
		UE_LOG(LogSkinMechanics, Error, TEXT("USkinSubsystem: failed to create a USkinSaveGame"));
		return false;
	}

	Save->PresetId = PresetId;
	Save->Overrides = Overrides;

	const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, Settings->SaveSlotName, Settings->SaveUserIndex);
	if (!bSaved)
	{
		UE_LOG(LogSkinMechanics, Error,
			TEXT("USkinSubsystem: failed to write save slot '%s'"), *Settings->SaveSlotName);
	}
	return bSaved;
}

void USkinSubsystem::ReloadSelection()
{
	const TMap<FGameplayTag, FName> Before = SnapshotResolved();
	LoadSelectionFromSlot();
	BroadcastChangesSince(Before);
}
