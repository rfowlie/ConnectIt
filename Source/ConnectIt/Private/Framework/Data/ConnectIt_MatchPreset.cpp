// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Data/ConnectIt_MatchPreset.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "AI/ConnectIt_AIDirector.h"
#include "Misc/DataValidation.h"


#if WITH_EDITOR
EDataValidationResult UConnectIt_MatchPreset::IsDataValid(
    FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (!IsValid(PlayerLoadout))
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_MatchPreset: No PlayerLoadout set.")));
        Result = EDataValidationResult::Invalid;
    }

    // Not an error: versus presets have no AI
    if (!IsValid(AIDirector))
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_MatchPreset: No AI Director set -- played against the AI, this "
            "preset gets the default director, which has no loadout.")));
    }
    else if (AIDirector->IsDataValid(Context) == EDataValidationResult::Invalid)
    {
        Result = EDataValidationResult::Invalid;
    }

    // No defaults and no borrowing between roles: each preset sets its own.
    if (Rules.PlayerWinGroups.IsEmpty())
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_MatchPreset: No Rules > Player Win Groups set -- "
            "the player can never win.")));
        Result = EDataValidationResult::Invalid;
    }

    // Not invalid: versus presets have no AI -- but if one plays, an empty
    // list means it cannot win.
    if (Rules.AIWinGroups.IsEmpty())
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_MatchPreset: No Rules > AI Win Groups set -- an AI "
            "opponent playing this preset can never win. Set them if it can be "
            "played against an AI.")));
    }

    // A group with no conditions is never satisfied, so it blocks every group after it
    const auto CheckGroups = [&Context, &Result](const TCHAR* Side, const TArray<FConnectItWinConditionGroup>& Groups)
    {
        for (int32 Index = 0; Index < Groups.Num(); Index++)
        {
            if (Groups[Index].IsEmpty())
            {
                Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                    "ConnectIt_MatchPreset: %s win group %d ('%s') has no conditions -- "
                    "it can never be completed, so that side can never win."),
                    Side, Index, *Groups[Index].Name.ToString())));
                Result = EDataValidationResult::Invalid;
            }
        }
    };
    CheckGroups(TEXT("Player"), Rules.PlayerWinGroups);
    CheckGroups(TEXT("AI"), Rules.AIWinGroups);

    return Result;
}
#endif
