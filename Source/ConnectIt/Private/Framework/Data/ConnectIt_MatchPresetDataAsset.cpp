// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Data/ConnectIt_MatchPresetDataAsset.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "AI/ConnectIt_AIDirector.h"
#include "AI/ConnectIt_PlayerDirector.h"
#include "AI/ConnectIt_LevelDirector.h"
#include "Board/Rules/Conditions/ConnectIt_MatchConditionDataAsset.h"
#include "Misc/DataValidation.h"


#if WITH_EDITOR
EDataValidationResult UConnectIt_MatchPresetDataAsset::IsDataValid(
    FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    // Each director class's defaults are its setup: validate those
    if (!PlayerDirectorClass)
    {
        Context.AddError(FText::FromString(TEXT(
            "ConnectIt_MatchPresetDataAsset: No Player Director Class -- players would have no actions.")));
        Result = EDataValidationResult::Invalid;
    }
    else if (GetDefault<UConnectIt_PlayerDirector>(PlayerDirectorClass)->IsDataValid(Context) == EDataValidationResult::Invalid)
    {
        Result = EDataValidationResult::Invalid;
    }

    // Not an error: versus presets have no AI
    if (!AIDirectorClass)
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_MatchPresetDataAsset: No AI Director Class -- played against the AI, this "
            "preset gets the default director, which has no loadout.")));
    }
    else if (GetDefault<UConnectIt_AIDirector>(AIDirectorClass)->IsDataValid(Context) == EDataValidationResult::Invalid)
    {
        Result = EDataValidationResult::Invalid;
    }

    // The Level director holds both sides' goals: without one nobody can win
    if (!LevelDirectorClass)
    {
        Context.AddError(FText::FromString(TEXT(
            "ConnectIt_MatchPresetDataAsset: No Level Director Class -- the match has no goals (the Level director holds "
            "both sides' win and lose conditions). Use a generic one for a standard way of playing.")));
        Result = EDataValidationResult::Invalid;
    }
    else if (GetDefault<UConnectIt_LevelDirector>(LevelDirectorClass)->IsDataValid(Context) == EDataValidationResult::Invalid)
    {
        Result = EDataValidationResult::Invalid;
    }

    // What the directors' conditions say about this preset's rules (e.g. a
    // condition needing the scoring rule's minimum)
    if (PlayerDirectorClass)
    {
        GetDefault<UConnectIt_PlayerDirector>(PlayerDirectorClass)->AddConditionRuleWarnings(Rules, Context);
    }
    if (AIDirectorClass)
    {
        GetDefault<UConnectIt_AIDirector>(AIDirectorClass)->AddConditionRuleWarnings(Rules, Context);
    }
    if (LevelDirectorClass)
    {
        GetDefault<UConnectIt_LevelDirector>(LevelDirectorClass)->AddConditionRuleWarnings(Rules, Context);
    }

    return Result;
}
#endif
