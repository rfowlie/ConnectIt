// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_AIProfile.h"
#include "AI/ConnectIt_AIStrategy.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Misc/DataValidation.h"


#if WITH_EDITOR
EDataValidationResult UConnectIt_AIProfile::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (!IsValid(Loadout))
    {
        Context.AddError(FText::FromString(TEXT(
            "ConnectIt_AIProfile: No Loadout set -- every move this AI makes "
            "would be rejected.")));
        Result = EDataValidationResult::Invalid;
    }

    if (!IsValid(Strategy))
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_AIProfile: No Strategy set -- the AI controller will "
            "fall back to a default MinMax strategy.")));
    }

    return Result;
}
#endif
