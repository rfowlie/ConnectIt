// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Board/ConnectIt_TileRegistry.h"
#include "Grid/ConnectIt_GridPiece.h"
#include "Misc/DataValidation.h"
#include "Registry/GridDefinition.h"


#if WITH_EDITOR
EDataValidationResult UConnectIt_LevelConfigDataAsset::IsDataValid(
    FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (!IsValid(PlayerLoadout))
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_LevelConfigDataAsset: No PlayerLoadout set.")));
        Result = EDataValidationResult::Invalid;
    }

    if (!IsValid(EnemyLoadout))
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_LevelConfigDataAsset: No EnemyLoadout set.")));
        Result = EDataValidationResult::Invalid;
    }

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

    return Result;
}
#endif
