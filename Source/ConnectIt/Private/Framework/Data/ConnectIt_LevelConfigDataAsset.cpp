// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Board/ConnectIt_TileRegistry.h"
#include "Board/Actor/ConnectIt_PieceBase.h"
#include "Board/ConnectIt_BoardVisualizer.h"
#include "Board/ConnectIt_EventVisual.h"
#include "Misc/DataValidation.h"
#include "Registry/GridDefinition.h"


#if WITH_EDITOR
EDataValidationResult UConnectIt_LevelConfigDataAsset::IsDataValid(
    FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (!PieceActorClass)
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_LevelConfigDataAsset: No PieceActorClass set.")));
        Result = EDataValidationResult::Invalid;
    }

    if (!IsValid(TileRegistry))
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_LevelConfigDataAsset: No TileRegistry template set.")));
        Result = EDataValidationResult::Invalid;
    }

    // PieceRegistry is intentionally not validated here -- not every level
    // needs one authored yet (UGridPieceRegistryBase::InitialiseRegistry()
    // is still a no-op stub).

    if (!IsValid(GridDefinition))
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_LevelConfigDataAsset: No GridDefinition template set -- "
            "TileRegistry/PieceRegistry position mapping will not work.")));
        Result = EDataValidationResult::Invalid;
    }

    // Event visuals: one per event kind, each a usable class
    TArray<FString> Errors;
    TArray<FString> Warnings;
    AConnectIt_BoardVisualizer::BuildEventVisualMap(EventVisuals, &Errors, &Warnings);
    for (const FString& Error : Errors)
    {
        Context.AddError(FText::FromString(TEXT("ConnectIt_LevelConfigDataAsset: ") + Error));
        Result = EDataValidationResult::Invalid;
    }
    for (const FString& Warning : Warnings)
    {
        Context.AddWarning(FText::FromString(TEXT("ConnectIt_LevelConfigDataAsset: ") + Warning));
    }

    return Result;
}
#endif
