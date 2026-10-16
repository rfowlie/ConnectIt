// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_AIProfile.h"
#include "AI/ConnectIt_AIStrategy.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Board/Operations/ConnectIt_BoardOperation.h"
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

    // Does what the strategy can play match what the loadout allows?
    const FGameplayTagContainer Supported =
        IsValid(Strategy) ? Strategy->GetSupportedRequestTypes() : FGameplayTagContainer();
    if (IsValid(Loadout) && !Supported.IsEmpty())
    {
        FGameplayTagContainer Granted;
        for (const FGameplayTag& RequestType : FConnectItBoardOperation::GetAllRequestTypes())
        {
            if (UConnectIt_AIStrategy::LoadoutGrantsRequestType(Loadout, RequestType))
            {
                Granted.AddTag(RequestType);
            }
        }

        if (!Granted.HasAnyExact(Supported))
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_AIProfile: the strategy only plays %s, and the loadout "
                "grants none of it -- this AI can never move."),
                *Supported.ToStringSimple())));
            Result = EDataValidationResult::Invalid;
        }

        FGameplayTagContainer NeverPlayed;
        for (const FGameplayTag& RequestType : Granted)
        {
            if (!Supported.HasTagExact(RequestType))
            {
                NeverPlayed.AddTag(RequestType);
            }
        }
        if (!NeverPlayed.IsEmpty())
        {
            Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_AIProfile: the loadout grants %s, which the strategy "
                "never plays."),
                *NeverPlayed.ToStringSimple())));
        }
    }

    return Result;
}
#endif
