// Fill out your copyright notice in the Description page of Project Settings.

#include "Adventure/ConnectIt_MapNodeDataAsset.h"
#include "Adventure/ConnectIt_SkillShardTable.h"
#include "AI/ConnectIt_PlayerDirector.h"
#include "Framework/Data/ConnectIt_MatchPreset.h"
#include "Misc/DataValidation.h"

FConnectItNodeRequirementCheck UConnectIt_MapNodeDataAsset::CheckRequirements(
    const FConnectItAdventureProgress& Progress, const UConnectIt_SkillShardTable* Table) const
{
    FConnectItNodeRequirementCheck Check;

    for (const FGameplayTag& Skill : Gate.RequiredSkills)
    {
        if (Progress.GetNumberedUses(Skill, Table) < 1)
        {
            Check.MissingSkills.AddTag(Skill);
        }
    }

    if (!Progress.IsSealBroken(NodeId))
    {
        for (const FGameplayTag& Seal : Gate.RequiredSeals)
        {
            if (!Progress.SpecificSeals.HasTagExact(Seal))
            {
                Check.MissingSeals.AddTag(Seal);
            }
        }
        Check.MissingGeneralSeals = FMath::Max(0, Gate.GeneralSeals - Progress.GeneralSeals);
    }
    return Check;
}

void UConnectIt_MapNodeDataAsset::PostInitProperties()
{
    Super::PostInitProperties();

    // A new asset gets its id here; a loaded one keeps its saved id (loading
    // sets it after this), and the class default object has none
    if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad | RF_WasLoaded) && !NodeId.IsValid())
    {
        NodeId = FGuid::NewGuid();
    }
}

void UConnectIt_MapNodeDataAsset::PostDuplicate(bool bDuplicateForPIE)
{
    Super::PostDuplicate(bDuplicateForPIE);

    // A copy is another node: two assets sharing an id would share progress
    if (!bDuplicateForPIE)
    {
        NodeId = FGuid::NewGuid();
    }
}

#if WITH_EDITOR
EDataValidationResult UConnectIt_MapNodeDataAsset::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);
    const FString Name = GetName();

    auto Error = [&Context, &Result](const FString& Message)
    {
        Context.AddError(FText::FromString(Message));
        Result = EDataValidationResult::Invalid;
    };

    if (!NodeId.IsValid())
    {
        Error(FString::Printf(TEXT("ConnectIt_MapNodeDataAsset %s: no Node Id -- the save can't refer to it."), *Name));
    }

    const bool bHasLevel = !Level.IsNull();
    if (!bHasLevel && !IsSubMap())
    {
        Error(FString::Printf(TEXT("ConnectIt_MapNodeDataAsset %s: neither a Level nor a Sub Map -- it plays nothing."), *Name));
    }
    else if (bHasLevel && IsSubMap())
    {
        Error(FString::Printf(TEXT("ConnectIt_MapNodeDataAsset %s: both a Level and a Sub Map -- set one."), *Name));
    }

    // A closed node plays its preset's loadout, so the preset must have one
    if (bHasLevel && SkillDesign == EConnectItSkillDesign::Closed)
    {
        const UConnectIt_MatchPreset* Preset = MatchPreset.LoadSynchronous();
        if (!Preset)
        {
            Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_MapNodeDataAsset %s: closed, with no Match Preset -- the level's own preset must give "
                "the loadout (not checked here)."), *Name)));
        }
        else
        {
            const UConnectIt_PlayerDirector* Director = Preset->PlayerDirectorClass
                ? Preset->PlayerDirectorClass->GetDefaultObject<UConnectIt_PlayerDirector>() : nullptr;
            if (!Director || !Director->StartingLoadout)
            {
                Error(FString::Printf(TEXT(
                    "ConnectIt_MapNodeDataAsset %s: closed, but preset %s's Player director gives no loadout."),
                    *Name, *Preset->GetName()));
            }
        }
    }

    for (const TSoftObjectPtr<UConnectIt_MapNodeDataAsset>& Unlocked : Unlocks)
    {
        if (Unlocked.ToSoftObjectPath() == FSoftObjectPath(this))
        {
            Error(FString::Printf(TEXT("ConnectIt_MapNodeDataAsset %s: unlocks itself."), *Name));
        }
        else if (Unlocked.IsNull())
        {
            Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_MapNodeDataAsset %s: an empty Unlocks entry."), *Name)));
        }
    }
    return Result;
}
#endif
