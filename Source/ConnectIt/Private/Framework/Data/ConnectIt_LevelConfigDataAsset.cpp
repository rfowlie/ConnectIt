// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Misc/DataValidation.h"

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

    return Result;
}
#endif
