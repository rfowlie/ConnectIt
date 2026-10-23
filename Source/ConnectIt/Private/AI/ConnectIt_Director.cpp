// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_Director.h"
#include "AI/ConnectIt_LevelDirector.h"
#include "AI/ConnectIt_PlayerDirector.h"
#include "AI/ConnectIt_AIDirector.h"
#include "Algo/StableSort.h"
#include "Engine/World.h"
#include "Framework/GameMode/ConnectIt_GameMode.h"
#include "Board/Rules/Conditions/ConnectIt_MatchConditionAsset.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Misc/DataValidation.h"


UWorld* UConnectIt_Director::GetWorld() const
{
    if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)) return nullptr;
    const UObject* Outer = GetOuter();
    return Outer ? Outer->GetWorld() : nullptr;
}

AConnectIt_GameMode* UConnectIt_Director::GetGameMode() const
{
    const UWorld* World = GetWorld();
    return World ? Cast<AConnectIt_GameMode>(World->GetAuthGameMode()) : nullptr;
}

UConnectIt_LevelDirector* UConnectIt_Director::GetLevelDirector() const
{
    const AConnectIt_GameMode* GameMode = GetGameMode();
    return GameMode ? GameMode->GetLevelDirector() : nullptr;
}

UConnectIt_PlayerDirector* UConnectIt_Director::GetPlayerDirector(int32 Faction) const
{
    const AConnectIt_GameMode* GameMode = GetGameMode();
    return GameMode ? GameMode->GetPlayerDirector(Faction) : nullptr;
}

UConnectIt_AIDirector* UConnectIt_Director::GetAIDirector() const
{
    const AConnectIt_GameMode* GameMode = GetGameMode();
    return GameMode ? GameMode->GetAIDirector() : nullptr;
}

TArray<UConnectIt_Director*> UConnectIt_Director::InDispatchOrder(
    UConnectIt_LevelDirector* Level,
    const TArray<UConnectIt_PlayerDirector*>& Players,
    UConnectIt_AIDirector* AI)
{
    TArray<UConnectIt_Director*> Ordered;
    if (IsValid(Level)) Ordered.Add(Level);

    TArray<UConnectIt_PlayerDirector*> SortedPlayers;
    for (UConnectIt_PlayerDirector* Player : Players)
    {
        if (IsValid(Player)) SortedPlayers.Add(Player);
    }
    Algo::StableSortBy(SortedPlayers, [](const UConnectIt_PlayerDirector* Player) { return Player->GetFaction(); });
    for (UConnectIt_PlayerDirector* Player : SortedPlayers)
    {
        Ordered.Add(Player);
    }

    if (IsValid(AI)) Ordered.Add(AI);
    return Ordered;
}

#if WITH_EDITOR
void UConnectIt_Director::ValidateConditionList(const UObject* Owner, const TCHAR* ListName,
    const TArray<TObjectPtr<UConnectIt_MatchConditionAsset>>& List,
    FDataValidationContext& Context, EDataValidationResult& Result)
{
    for (int32 Index = 0; Index < List.Num(); Index++)
    {
        const UConnectIt_MatchConditionAsset* Asset = List[Index];
        if (!IsValid(Asset))
        {
            Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                "%s: %s entry %d is empty -- it is skipped."), *GetNameSafe(Owner), ListName, Index)));
        }
        else if (Asset->Conditions.IsEmpty())
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "%s: %s entry %d ('%s') has no conditions -- it can never be met."),
                *GetNameSafe(Owner), ListName, Index, *Asset->GetName())));
            Result = EDataValidationResult::Invalid;
        }
    }
}

void UConnectIt_Director::AddConditionListRuleWarnings(const UObject* Owner, const TCHAR* ListName,
    const TArray<TObjectPtr<UConnectIt_MatchConditionAsset>>& List,
    const FConnectItRuleSet& Rules, FDataValidationContext& Context)
{
    for (int32 Index = 0; Index < List.Num(); Index++)
    {
        const UConnectIt_MatchConditionAsset* Asset = List[Index];
        if (!IsValid(Asset)) continue;

        for (const TInstancedStruct<FConnectItMatchCondition>& Entry : Asset->Conditions)
        {
            const FConnectItMatchCondition* Condition = Entry.GetPtr();
            if (!Condition) continue;

            TArray<FText> Warnings;
            Condition->GetValidationWarnings(&Rules, Warnings);
            for (const FText& Warning : Warnings)
            {
                Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                    "%s: %s entry %d ('%s'): %s"),
                    *GetNameSafe(Owner), ListName, Index, *Asset->GetName(), *Warning.ToString())));
            }
        }
    }
}
#endif
