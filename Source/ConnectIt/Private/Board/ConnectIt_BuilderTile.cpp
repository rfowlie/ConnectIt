// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/ConnectIt_BuilderTile.h"
#include "Board/Powers/ConnectIt_TileCatalog.h"
#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Framework/Data/ConnectIt_WorldSettings.h"
#include "Registry/GridDefinition.h"
#include "ConnectIt_GameplayTags.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif


FConnectItTileInitialData AConnectIt_BuilderTile::GetInitialTileData_Implementation() const
{
    FConnectItTileInitialData Data = TileData;
    if (IsValid(ChainPartnerTile))
    {
        const UConnectIt_LevelConfigDataAsset* LevelConfig = AConnectIt_WorldSettings::FindLevelConfig(this);
        if (LevelConfig && IsValid(LevelConfig->GridDefinition))
        {
            Data.bHasChainPartner = true;
            Data.ChainPartner = LevelConfig->GridDefinition->WorldToGridPosition(ChainPartnerTile->GetActorLocation());
        }
    }
    return Data;
}

void AConnectIt_BuilderTile::ApplyLayoutCell_Implementation(const FConnectItTileInitialData& Data, FGameplayTag InVisualVariant)
{
    TileData = Data;
    VisualVariant = InVisualVariant;
}

#if WITH_EDITOR
EDataValidationResult AConnectIt_BuilderTile::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    // A placed tile: its tag must be one its level's catalog defines (the
    // class's own defaults are checked against the project default)
    if (TileData.TilePowerTag.IsValid())
    {
        const UConnectIt_TileCatalog* Catalog = GetWorld()
            ? UConnectIt_TileCatalog::FindFor(this)
            : UConnectIt_TileCatalog::GetProjectDefault();
        if (Catalog && !Catalog->Defines(TileData.TilePowerTag))
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_BuilderTile '%s': power tag %s isn't in tile catalog '%s'."),
                *GetName(), *TileData.TilePowerTag.ToString(), *Catalog->GetName())));
            Result = EDataValidationResult::Invalid;
        }
    }
    // A chain partner only means something on a Chained tile
    const bool bChained = TileData.TilePowerTag == ConnectIt_TilePower_Chained;
    const bool bHasPartner = IsValid(ChainPartnerTile) || TileData.bHasChainPartner;
    if (bHasPartner && !bChained)
    {
        Context.AddWarning(FText::FromString(FString::Printf(TEXT(
            "ConnectIt_BuilderTile '%s': a chain partner is set but the tile isn't Chained -- it is ignored."),
            *GetName())));
    }
    if (ChainPartnerTile == this)
    {
        Context.AddError(FText::FromString(FString::Printf(TEXT(
            "ConnectIt_BuilderTile '%s': its chain partner is itself."), *GetName())));
        Result = EDataValidationResult::Invalid;
    }
    else if (bChained && IsValid(ChainPartnerTile) && ChainPartnerTile->TileData.TilePowerTag == ConnectIt_TilePower_Chained)
    {
        Context.AddWarning(FText::FromString(FString::Printf(TEXT(
            "ConnectIt_BuilderTile '%s': its partner '%s' is Chained too -- a chain of chains isn't tested yet."),
            *GetName(), *ChainPartnerTile->GetName())));
    }
    return Result;
}
#endif
