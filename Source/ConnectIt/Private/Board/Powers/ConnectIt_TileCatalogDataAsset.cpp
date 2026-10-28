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


TSubclassOf<AConnectIt_TileBase> UConnectIt_TileCatalogDataAsset::GetTileClassFor(FGameplayTag TilePowerTag) const
{
    if (!TilePowerTag.IsValid()) return DefaultTileClass;
    const FConnectItTileCatalogEntry* Entry = Tiles.Find(TilePowerTag);
    return Entry ? Entry->TileClass : nullptr;
}

TArray<FConnectItTilePower> UConnectIt_TileCatalogDataAsset::MakeTilePowers() const
{
    TArray<FConnectItTilePower> Powers;
    Powers.Reserve(Tiles.Num());
    for (const TPair<FGameplayTag, FConnectItTileCatalogEntry>& Pair : Tiles)
    {
        if (!Pair.Key.IsValid()) continue;
        Powers.Add(Pair.Value.TilePower);
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

    for (const TPair<FGameplayTag, FConnectItTileCatalogEntry>& Pair : Tiles)
    {
        const FString Key = Pair.Key.ToString();
        if (!Pair.Key.IsValid())
        {
            Context.AddError(FText::FromString(TEXT("ConnectIt_TileCatalogDataAsset: an entry has no tag.")));
            Result = EDataValidationResult::Invalid;
            continue;
        }
        if (!Pair.Value.TileClass)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_TileCatalogDataAsset: %s has no Tile Class."), *Key)));
            Result = EDataValidationResult::Invalid;
        }
        if (!Pair.Value.TilePower.Effect.IsValid())
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_TileCatalogDataAsset: %s's power has no Effect."), *Key)));
            Result = EDataValidationResult::Invalid;
        }
        const FConnectItTilePower& Power = Pair.Value.TilePower;
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
        if (Power.Effect.IsValid() && Power.GetTag() != Pair.Key)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_TileCatalogDataAsset: %s's effect is the %s power -- the entry's tag must be its effect's."),
                *Key, *Power.GetTag().ToString())));
            Result = EDataValidationResult::Invalid;
        }
    }
    return Result;
}
#endif
