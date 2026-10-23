// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Powers/ConnectIt_TileCatalog.h"
#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Framework/Data/ConnectIt_ProjectSettings.h"
#include "Framework/Data/ConnectIt_WorldSettings.h"
#include "Tile/GridTileBase.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif


TSubclassOf<AGridTileBase> UConnectIt_TileCatalog::GetTileClassFor(FGameplayTag TilePowerTag) const
{
    if (!TilePowerTag.IsValid()) return PlainTileClass;
    const FConnectItTileCatalogEntry* Entry = Tiles.Find(TilePowerTag);
    return Entry ? Entry->TileClass : nullptr;
}

TArray<FConnectItTilePower> UConnectIt_TileCatalog::MakePowers() const
{
    TArray<FConnectItTilePower> Powers;
    Powers.Reserve(Tiles.Num());
    for (const TPair<FGameplayTag, FConnectItTileCatalogEntry>& Pair : Tiles)
    {
        if (!Pair.Key.IsValid()) continue;
        FConnectItTilePower& Power = Powers.Add_GetRef(Pair.Value.Power);
        Power.Name = Pair.Key;
    }
    return Powers;
}

const UConnectIt_TileCatalog* UConnectIt_TileCatalog::GetProjectDefault()
{
    const UConnectIt_ProjectSettings* Settings = GetDefault<UConnectIt_ProjectSettings>();
    return Settings ? Settings->DefaultTileCatalog.LoadSynchronous() : nullptr;
}

const UConnectIt_TileCatalog* UConnectIt_TileCatalog::FindFor(const UObject* WorldContextObject)
{
    const UConnectIt_LevelConfigDataAsset* LevelConfig = AConnectIt_WorldSettings::FindLevelConfig(WorldContextObject);
    if (LevelConfig && IsValid(LevelConfig->TileCatalog))
    {
        return LevelConfig->TileCatalog;
    }

    const UConnectIt_TileCatalog* Default = GetProjectDefault();
    if (!Default)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_TileCatalog: no tile catalog -- the level config names none and Project Settings > "
                 "ConnectIt has no Default Tile Catalog. Tiles have no powers and builder tiles aren't swapped."));
    }
    return Default;
}

#if WITH_EDITOR
EDataValidationResult UConnectIt_TileCatalog::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (!PlainTileClass)
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_TileCatalog: no Plain Tile Class -- tiles with no power have no actor to show them.")));
    }

    for (const TPair<FGameplayTag, FConnectItTileCatalogEntry>& Pair : Tiles)
    {
        const FString Key = Pair.Key.ToString();
        if (!Pair.Key.IsValid())
        {
            Context.AddError(FText::FromString(TEXT("ConnectIt_TileCatalog: an entry has no tag.")));
            Result = EDataValidationResult::Invalid;
            continue;
        }
        if (!Pair.Value.TileClass)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_TileCatalog: %s has no Tile Class."), *Key)));
            Result = EDataValidationResult::Invalid;
        }
        if (!Pair.Value.Power.Effect.IsValid())
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_TileCatalog: %s's power has no Effect."), *Key)));
            Result = EDataValidationResult::Invalid;
        }
        if (Pair.Value.Power.Name.IsValid() && Pair.Value.Power.Name != Pair.Key)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_TileCatalog: %s's power is named %s -- leave Name empty or the same as the entry's tag."),
                *Key, *Pair.Value.Power.Name.ToString())));
            Result = EDataValidationResult::Invalid;
        }
    }
    return Result;
}
#endif
