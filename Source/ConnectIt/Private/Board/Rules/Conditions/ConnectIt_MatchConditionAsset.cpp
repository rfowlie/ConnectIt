// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Rules/Conditions/ConnectIt_MatchConditionAsset.h"
#include "Misc/DataValidation.h"


#if WITH_EDITOR
EDataValidationResult UConnectIt_MatchConditionAsset::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (Conditions.IsEmpty())
    {
        Context.AddError(FText::FromString(TEXT(
            "ConnectIt_MatchConditionAsset: no Conditions -- this goal can never be completed, "
            "so a side listing it can never win.")));
        Result = EDataValidationResult::Invalid;
    }

    for (int32 Index = 0; Index < Conditions.Num(); Index++)
    {
        if (const FConnectItMatchCondition* Condition = Conditions[Index].GetPtr())
        {
            TArray<FText> Warnings;
            Condition->GetValidationWarnings(nullptr, Warnings);
            for (const FText& Warning : Warnings)
            {
                Context.AddWarning(Warning);
            }
        }

        if (!Conditions[Index].IsValid())
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_MatchConditionAsset: condition %d is empty -- it can't be met, so this goal "
                "can never be completed."), Index)));
            Result = EDataValidationResult::Invalid;
        }
    }

    return Result;
}
#endif
