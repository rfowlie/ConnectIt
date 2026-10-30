// Fill out your copyright notice in the Description page of Project Settings.

#include "Adventure/ConnectIt_AdventureProgress.h"
#include "Adventure/ConnectIt_ActionShardTableDataAsset.h"
#include "Adventure/ConnectIt_MapNodeDataAsset.h"
#include "Action/ItemActionInventoryDataAsset.h"
#include "Action/TurnBasedPlayerAction.h"
#include "Action/ActionLoadoutDataAsset.h"

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

    return bWon && CompleteNode(Node, Items);
}

bool FConnectItAdventureProgress::CompleteNode(const UConnectIt_MapNodeDataAsset& Node, const UItemActionInventoryDataAsset* Items)
{
    if (IsCompleted(Node.NodeId)) return false;
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

TArray<FPermanentActionConfig> FConnectItAdventureProgress::BuildOpenPermanentActions(
    const UActionLoadoutDataAsset* Loadout, const UConnectIt_ActionShardTableDataAsset* Table) const
{
    TArray<FPermanentActionConfig> Actions;
    const auto IsPriced = [Table](const TSubclassOf<UTurnBasedPlayerAction>& ActionClass)
    {
        return Table && Table->Actions.Contains(UTurnBasedPlayerAction::GetTagForClass(ActionClass));
    };

    // Everyone's actions (no shard price), as the loadout sets them up
    if (Loadout)
    {
        for (const FPermanentActionConfig& Entry : Loadout->PermanentActions)
        {
            if (Entry.ActionClass && !IsPriced(Entry.ActionClass))
            {
                Actions.Add(Entry);
            }
        }
    }

    // Each action the shards have given uses of
    if (Table)
    {
        for (const TPair<FGameplayTag, FConnectItActionShardCosts>& Priced : Table->Actions)
        {
            const int32 Uses = GetPermanentUses(Priced.Key, Table);
            if (Uses < 1 || !Priced.Value.ActionClass) continue;

            FPermanentActionConfig Config;
            const FPermanentActionConfig* LoadoutEntry = Loadout ? Loadout->PermanentActions.FindByPredicate(
                [&Priced](const FPermanentActionConfig& Entry) { return Entry.ActionClass == Priced.Value.ActionClass; }) : nullptr;
            if (LoadoutEntry)
            {
                Config = *LoadoutEntry;
            }
            Config.ActionClass = Priced.Value.ActionClass;
            Config.MatchUses = Uses;
            Actions.Add(Config);
        }
    }
    return Actions;
}

TArray<FItemActionEntry> FConnectItAdventureProgress::MakeInventory(const UConnectIt_ActionShardTableDataAsset* Table,
    const UItemActionInventoryDataAsset* Items) const
{
    TArray<FItemActionEntry> Inventory;
    for (const TPair<FGameplayTag, int32>& Held : ItemUses)
    {
        if (Held.Value < 1) continue;
        const TSubclassOf<UTurnBasedPlayerAction> ActionClass = FindActionClass(Held.Key, Table, Items);
        if (!ActionClass)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ConnectIt_AdventureProgress: item %s has no action class (neither the shard table nor the item "
                     "inventory asset names it) -- left out of the match"), *Held.Key.ToString());
            continue;
        }
        FItemActionEntry& Entry = Inventory.AddDefaulted_GetRef();
        Entry.ActionClass = ActionClass;
        Entry.Count = Held.Value;
    }
    return Inventory;
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
