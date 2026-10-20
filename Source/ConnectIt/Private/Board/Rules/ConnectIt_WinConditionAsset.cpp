// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Rules/ConnectIt_WinConditionAsset.h"
#include "Misc/DataValidation.h"


#if WITH_EDITOR
EDataValidationResult UConnectIt_WinConditionAsset::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (Conditions.IsEmpty())
    {
        Context.AddError(FText::FromString(TEXT(
            "ConnectIt_WinConditionAsset: no Conditions -- this goal can never be completed, "
            "so a side listing it can never win.")));
        Result = EDataValidationResult::Invalid;
    }

    for (int32 Index = 0; Index < Conditions.Num(); Index++)
    {
        if (!Conditions[Index].IsValid())
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_WinConditionAsset: condition %d is empty -- it can't be met, so this goal "
                "can never be completed."), Index)));
            Result = EDataValidationResult::Invalid;
        }
    }

    return Result;
}
#endif
