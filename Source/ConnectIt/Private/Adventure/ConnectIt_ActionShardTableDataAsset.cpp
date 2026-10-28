// Fill out your copyright notice in the Description page of Project Settings.

#include "Adventure/ConnectIt_ActionShardTableDataAsset.h"
#include "Framework/Data/ConnectIt_ProjectSettings.h"
#include "Misc/DataValidation.h"

int32 FConnectItActionShardCosts::GetPermanentUses(int32 Shards) const
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

int32 UConnectIt_ActionShardTableDataAsset::GetPermanentUses(FGameplayTag Action, int32 Shards) const
{
    const FConnectItActionShardCosts* Entry = Actions.Find(Action);
    return Entry ? Entry->GetPermanentUses(Shards) : 0;
}

const UConnectIt_ActionShardTableDataAsset* UConnectIt_ActionShardTableDataAsset::Get()
{
    const UConnectIt_ProjectSettings* Settings = GetDefault<UConnectIt_ProjectSettings>();
    return Settings ? Settings->ActionShardTable.LoadSynchronous() : nullptr;
}

#if WITH_EDITOR
EDataValidationResult UConnectIt_ActionShardTableDataAsset::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    for (const TPair<FGameplayTag, FConnectItActionShardCosts>& Pair : Actions)
    {
        if (!Pair.Key.IsValid())
        {
            Context.AddError(FText::FromString(TEXT("ConnectIt_ActionShardTableDataAsset: an entry has no action tag.")));
            Result = EDataValidationResult::Invalid;
            continue;
        }

        const TArray<int32>& Costs = Pair.Value.ShardsPerUse;
        if (Costs.IsEmpty())
        {
            Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_ActionShardTableDataAsset: %s has no shard costs -- shards never give it a use."),
                *Pair.Key.ToString())));
        }
        for (int32 Index = 1; Index < Costs.Num(); Index++)
        {
            if (Costs[Index] < Costs[Index - 1])
            {
                Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                    "ConnectIt_ActionShardTableDataAsset: %s's shard costs fall (use %d costs %d, use %d costs %d) -- "
                    "normally each use costs at least as much as the last."),
                    *Pair.Key.ToString(), Index, Costs[Index - 1], Index + 1, Costs[Index])));
                break;
            }
        }
    }
    return Result;
}
#endif
