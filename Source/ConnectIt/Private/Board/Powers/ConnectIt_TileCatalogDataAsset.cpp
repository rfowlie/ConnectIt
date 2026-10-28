// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Powers/ConnectIt_TileCatalogDataAsset.h"
#include "Framework/Data/ConnectIt_ProjectSettings.h"
#include "Framework/Data/ConnectIt_WorldSettings.h"
#include "Board/Actor/ConnectIt_TileBase.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif


const FConnectItTileCatalogEntry* UConnectIt_TileCatalogDataAsset::FindEntry(const FGameplayTag& TilePowerTag) const
{
    if (!TilePowerTag.IsValid()) return nullptr;
    if (!bEntryByTagBuilt)
    {
        EntryByTag.Reset();
        for (int32 Index = 0; Index < Entries.Num(); Index++)
        {
            const FGameplayTag Tag = Entries[Index].TilePower.GetTag();
            if (Tag.IsValid() && !EntryByTag.Contains(Tag))
            {
                EntryByTag.Add(Tag, Index);
            }
        }
        bEntryByTagBuilt = true;
    }
    const int32* Index = EntryByTag.Find(TilePowerTag);
    return Index && Entries.IsValidIndex(*Index) ? &Entries[*Index] : nullptr;
}

TSubclassOf<AConnectIt_TileBase> UConnectIt_TileCatalogDataAsset::GetTileClassFor(FGameplayTag TilePowerTag) const
{
    if (!TilePowerTag.IsValid()) return DefaultTileClass;
    const FConnectItTileCatalogEntry* Entry = FindEntry(TilePowerTag);
    return Entry ? Entry->TileClass : nullptr;
}

TArray<FConnectItTilePower> UConnectIt_TileCatalogDataAsset::MakeTilePowers() const
{
    // One per tag: the entry the lookup finds (a repeat is a validation error)
    TArray<FConnectItTilePower> Powers;
    Powers.Reserve(Entries.Num());
    for (const FConnectItTileCatalogEntry& Entry : Entries)
    {
        const FGameplayTag Tag = Entry.TilePower.GetTag();
        if (Tag.IsValid() && FindEntry(Tag) == &Entry)
        {
            Powers.Add(Entry.TilePower);
        }
    }
    return Powers;
}

const UConnectIt_TileCatalogDataAsset* UConnectIt_TileCatalogDataAsset::GetProjectDefault()
{
    const UConnectIt_ProjectSettings* Settings = GetDefault<UConnectIt_ProjectSettings>();
    return Settings ? Settings->DefaultTileCatalog.LoadSynchronous() : nullptr;
}

const UConnectIt_TileCatalogDataAsset* UConnectIt_TileCatalogDataAsset::FindFor(const UObject* WorldContextObject)
{
    const UWorld* World = GEngine && IsValid(WorldContextObject)
        ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
        : nullptr;
    const AConnectIt_WorldSettings* WorldSettings = World ? Cast<AConnectIt_WorldSettings>(World->GetWorldSettings()) : nullptr;
    if (WorldSettings && IsValid(WorldSettings->TileCatalog))
    {
        return WorldSettings->TileCatalog;
    }

    const UConnectIt_TileCatalogDataAsset* Default = GetProjectDefault();
    if (!Default)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_TileCatalogDataAsset: no tile catalog -- the map's World Settings name none and Project Settings > "
                 "ConnectIt has no Default Tile Catalog. Tiles have no powers and builder tiles aren't swapped."));
    }
    return Default;
}

#if WITH_EDITOR
EDataValidationResult UConnectIt_TileCatalogDataAsset::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (!DefaultTileClass)
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_TileCatalogDataAsset: no Default Tile Class -- tiles with no power have no actor to show them.")));
    }

    TSet<FGameplayTag> Seen;
    for (int32 Index = 0; Index < Entries.Num(); Index++)
    {
        const FConnectItTileCatalogEntry& Entry = Entries[Index];
        const FConnectItTilePower& Power = Entry.TilePower;
        const FGameplayTag Tag = Power.GetTag();
        const FString Key = Tag.IsValid() ? Tag.ToString() : FString::Printf(TEXT("Entry %d"), Index);
        if (!Tag.IsValid())
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_TileCatalogDataAsset: %s has no Effect (an effect names its power tag)."), *Key)));
            Result = EDataValidationResult::Invalid;
        }
        else if (Seen.Contains(Tag))
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_TileCatalogDataAsset: %s is defined twice (entry %d) -- the first is used."), *Key, Index)));
            Result = EDataValidationResult::Invalid;
        }
        Seen.Add(Tag);
        if (!Entry.TileClass)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_TileCatalogDataAsset: %s has no Tile Class."), *Key)));
            Result = EDataValidationResult::Invalid;
        }
        if (Power.HasTrigger(EConnectItTilePowerTrigger::Sync) && Power.Triggers != (1 << static_cast<int32>(EConnectItTilePowerTrigger::Sync)))
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_TileCatalogDataAsset: %s is Sync and has other triggers -- a Sync power has only Sync."), *Key)));
            Result = EDataValidationResult::Invalid;
        }
        if (Power.Triggers == 0)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_TileCatalogDataAsset: %s has no trigger -- it never fires."), *Key)));
            Result = EDataValidationResult::Invalid;
        }
    }
    return Result;
}

void UConnectIt_TileCatalogDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    bEntryByTagBuilt = false;
}
#endif
