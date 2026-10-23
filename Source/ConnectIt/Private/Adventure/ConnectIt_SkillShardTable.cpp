// Fill out your copyright notice in the Description page of Project Settings.

#include "Adventure/ConnectIt_SkillShardTable.h"
#include "Framework/Data/ConnectIt_ProjectSettings.h"
#include "Misc/DataValidation.h"

int32 FConnectItSkillShardCosts::GetNumberedUses(int32 Shards) const
{
    // Pay for each use in turn until the shards run out
    int32 Uses = 0;
    for (const int32 Cost : ShardsPerUse)
    {
        Shards -= FMath::Max(Cost, 1);
        if (Shards < 0) break;
        Uses++;
    }
    return Uses;
}

int32 UConnectIt_SkillShardTable::GetNumberedUses(FGameplayTag Skill, int32 Shards) const
{
    const FConnectItSkillShardCosts* Entry = Skills.Find(Skill);
    return Entry ? Entry->GetNumberedUses(Shards) : 0;
}

const UConnectIt_SkillShardTable* UConnectIt_SkillShardTable::Get()
{
    const UConnectIt_ProjectSettings* Settings = GetDefault<UConnectIt_ProjectSettings>();
    return Settings ? Settings->SkillShardTable.LoadSynchronous() : nullptr;
}

#if WITH_EDITOR
EDataValidationResult UConnectIt_SkillShardTable::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    for (const TPair<FGameplayTag, FConnectItSkillShardCosts>& Pair : Skills)
    {
        if (!Pair.Key.IsValid())
        {
            Context.AddError(FText::FromString(TEXT("ConnectIt_SkillShardTable: an entry has no skill tag.")));
            Result = EDataValidationResult::Invalid;
            continue;
        }

        const TArray<int32>& Costs = Pair.Value.ShardsPerUse;
        if (Costs.IsEmpty())
        {
            Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_SkillShardTable: %s has no shard costs -- shards never give it a use."),
                *Pair.Key.ToString())));
        }
        for (int32 Index = 1; Index < Costs.Num(); Index++)
        {
            if (Costs[Index] < Costs[Index - 1])
            {
                Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                    "ConnectIt_SkillShardTable: %s's shard costs fall (use %d costs %d, use %d costs %d) -- "
                    "normally each use costs at least as much as the last."),
                    *Pair.Key.ToString(), Index, Costs[Index - 1], Index + 1, Costs[Index])));
                break;
            }
        }
    }
    return Result;
}
#endif
