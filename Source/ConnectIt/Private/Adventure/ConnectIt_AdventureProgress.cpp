// Fill out your copyright notice in the Description page of Project Settings.

#include "Adventure/ConnectIt_AdventureProgress.h"
#include "Adventure/ConnectIt_ActionShardTableDataAsset.h"
#include "Adventure/ConnectIt_MapNodeDataAsset.h"
#include "Action/ItemActionInventoryDataAsset.h"
#include "Action/TurnBasedPlayerAction.h"

int32 FConnectItAdventureProgress::GetPermanentUses(const FGameplayTag& Action, const UConnectIt_ActionShardTableDataAsset* Table) const
{
    return Table ? Table->GetPermanentUses(Action, GetShards(Action)) : 0;
}

EConnectItNodeState FConnectItAdventureProgress::GetNodeState(const UConnectIt_MapNodeDataAsset& Node, bool bRevealed,
    const UConnectIt_ActionShardTableDataAsset* Table, FConnectItNodeRequirementCheck& OutCheck) const
{
    OutCheck = Node.CheckRequirements(*this, Table);
    if (IsCompleted(Node.NodeId)) return EConnectItNodeState::Completed;
    if (!bRevealed) return EConnectItNodeState::Hidden;
    return OutCheck.IsMet() ? EConnectItNodeState::Available : EConnectItNodeState::Gated;
}

bool FConnectItAdventureProgress::CanBreakSeal(const UConnectIt_MapNodeDataAsset& Node, FString* OutWhyNot) const
{
    const auto WhyNot = [OutWhyNot](const FString& Reason)
    {
        if (OutWhyNot) *OutWhyNot = Reason;
        return false;
    };

    if (!Node.Gate.HasSeals()) return WhyNot(TEXT("it has no seal"));
    if (IsSealBroken(Node.NodeId)) return WhyNot(TEXT("its seal is already broken"));

    for (const FGameplayTag& Seal : Node.Gate.RequiredSeals)
    {
        if (!SpecificSeals.HasTagExact(Seal))
        {
            return WhyNot(FString::Printf(TEXT("the %s seal isn't held"), *Seal.ToString()));
        }
    }
    if (GeneralSeals < Node.Gate.GeneralSeals)
    {
        return WhyNot(FString::Printf(TEXT("%d general seal(s) needed, %d held"), Node.Gate.GeneralSeals, GeneralSeals));
    }
    return true;
}

bool FConnectItAdventureProgress::BreakSeal(const UConnectIt_MapNodeDataAsset& Node)
{
    if (!CanBreakSeal(Node)) return false;

    SpecificSeals.RemoveTags(Node.Gate.RequiredSeals);
    GeneralSeals -= Node.Gate.GeneralSeals;
    BrokenSeals.Add(Node.NodeId);
    return true;
}

bool FConnectItAdventureProgress::RecordResult(const UConnectIt_MapNodeDataAsset& Node, bool bWon,
    const TArray<FItemActionEntry>& Inventory, const UItemActionInventoryDataAsset* Items)
{
    // The inventory as the match left it (the save keys it by tag)
    ItemUses.Reset();
    for (const FItemActionEntry& Entry : Inventory)
    {
        const FGameplayTag Action = UTurnBasedPlayerAction::GetTagForClass(Entry.ActionClass);
        if (Action.IsValid() && Entry.Count > 0)
        {
            ItemUses.FindOrAdd(Action) += Entry.Count;
        }
    }
    LastPlayedNode = Node.NodeId;

    if (!bWon || IsCompleted(Node.NodeId)) return false;
    CompletedNodes.Add(Node.NodeId);

    const FConnectItNodeRewards& Rewards = Node.Rewards;
    for (const TPair<FGameplayTag, int32>& Reward : Rewards.Shards)
    {
        Shards.FindOrAdd(Reward.Key) += FMath::Max(Reward.Value, 0);
    }
    for (const TPair<FGameplayTag, int32>& Reward : Rewards.ItemUses)
    {
        const TSubclassOf<UTurnBasedPlayerAction> ActionClass = FindActionClass(Reward.Key, nullptr, Items);
        const int32 Cap = Items && ActionClass ? Items->GetMaxHeld(ActionClass) : 0;
        if (Cap <= 0)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ConnectIt_AdventureProgress: %s's reward of item %s skipped -- the item inventory asset doesn't "
                     "list it, so it can't be held"),
                *Node.GetName(), *Reward.Key.ToString());
            continue;
        }
        int32& Held = ItemUses.FindOrAdd(Reward.Key);
        Held = FMath::Min(Held + FMath::Max(Reward.Value, 0), Cap);
    }
    SpecificSeals.AppendTags(Rewards.Seals);
    GeneralSeals += FMath::Max(Rewards.GeneralSeals, 0);
    return true;
}

TSubclassOf<UTurnBasedPlayerAction> FConnectItAdventureProgress::FindActionClass(const FGameplayTag& Action,
    const UConnectIt_ActionShardTableDataAsset* Table, const UItemActionInventoryDataAsset* Items)
{
    if (!Action.IsValid()) return nullptr;

    const FConnectItActionShardCosts* Entry = Table ? Table->Actions.Find(Action) : nullptr;
    if (Entry && Entry->ActionClass)
    {
        return Entry->ActionClass;
    }
    if (Items)
    {
        for (const TPair<TSubclassOf<UTurnBasedPlayerAction>, int32>& Held : Items->MaxHeld)
        {
            if (Held.Key && UTurnBasedPlayerAction::GetTagForClass(Held.Key) == Action)
            {
                return Held.Key;
            }
        }
    }
    return nullptr;
}
