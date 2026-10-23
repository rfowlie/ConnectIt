// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/ConnectIt_BuilderTile.h"
#include "Board/Powers/ConnectIt_TileCatalog.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif


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
    return Result;
}
#endif
