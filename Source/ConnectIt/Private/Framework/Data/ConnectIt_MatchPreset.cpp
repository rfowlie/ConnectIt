// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Data/ConnectIt_MatchPreset.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "AI/ConnectIt_AIDirector.h"
#include "Board/Rules/ConnectIt_MatchConditionAsset.h"
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
    if (Rules.PlayerWinConditions.IsEmpty())
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_MatchPreset: No Rules > Player Win Conditions set -- "
            "the player can never win.")));
        Result = EDataValidationResult::Invalid;
    }

    // Not invalid: versus presets have no AI -- but if one plays, an empty
    // list means it cannot win.
    if (Rules.AIWinConditions.IsEmpty())
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_MatchPreset: No Rules > AI Win Conditions set -- an AI "
            "opponent playing this preset can never win. Set them if it can be "
            "played against an AI.")));
    }

    // A goal that is unset or has no conditions is never satisfied, so it
    // blocks every goal after it
    const auto CheckGoals = [&Context, &Result](const TCHAR* Side,
        const TArray<TObjectPtr<UConnectIt_MatchConditionAsset>>& Goals)
    {
        for (int32 Index = 0; Index < Goals.Num(); Index++)
        {
            if (!IsValid(Goals[Index]))
            {
                Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                    "ConnectIt_MatchPreset: %s win condition %d is empty -- it is skipped."),
                    Side, Index)));
            }
            else if (Goals[Index]->Conditions.IsEmpty())
            {
                Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                    "ConnectIt_MatchPreset: %s win condition %d ('%s') has no conditions -- "
                    "it can never be completed, so that side can never win."),
                    Side, Index, *Goals[Index]->GetName())));
                Result = EDataValidationResult::Invalid;
            }
        }
    };
    CheckGoals(TEXT("Player"), Rules.PlayerWinConditions);
    CheckGoals(TEXT("AI"), Rules.AIWinConditions);

    return Result;
}
#endif
