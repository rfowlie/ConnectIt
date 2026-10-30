// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "Action/ActionConfig.h"
#include "ConnectIt_AdventureProgress.generated.h"

class UConnectIt_ActionShardTableDataAsset;
class UConnectIt_MapNodeDataAsset;
class UItemActionInventoryDataAsset;
class UTurnBasedPlayerAction;
class UActionLoadoutDataAsset;
struct FConnectItNodeRequirementCheck;

// Where a node stands for a player
UENUM(BlueprintType)
enum class EConnectItNodeState : uint8
{
    // Not revealed on the hub yet
    Hidden,
    // Revealed and can be started
    Available,
    // Revealed, but needs its gate first: required actions, or seals to break
    Gated,
    // Won (no replays for now)
    Completed,
};


// A player's Adventure progress: what the save holds. Nodes are named by their
// data asset's NodeId (a GUID), so renaming or moving an asset keeps progress.
// Permanent uses aren't stored: they come from the shards through the action
// shard table (GetPermanentUses).
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItAdventureProgress
{
    GENERATED_BODY()

    // Nodes won
    UPROPERTY(BlueprintReadOnly, Category = "Adventure")
    TSet<FGuid> CompletedNodes;

    // Gated nodes whose seals were broken: open for good
    UPROPERTY(BlueprintReadOnly, Category = "Adventure")
    TSet<FGuid> BrokenSeals;

    // Shards per action (ConnectIt.PlayerAction.*)
    UPROPERTY(BlueprintReadOnly, Category = "Adventure")
    TMap<FGameplayTag, int32> Shards;

    // The inventory: item actions held per player action (one-time uses).
    // From level rewards and grants; never count towards a node's required
    // actions.
    UPROPERTY(BlueprintReadOnly, Category = "Adventure")
    TMap<FGameplayTag, int32> ItemUses;

    // Specific seals held (ConnectIt.Adventure.Seal.*), each opening the gate that
    // names it
    UPROPERTY(BlueprintReadOnly, Category = "Adventure")
    FGameplayTagContainer SpecificSeals;

    // General seals held: each opens any gate's general seal cost
    UPROPERTY(BlueprintReadOnly, Category = "Adventure")
    int32 GeneralSeals = 0;

    // The node played last (the hub focuses it)
    UPROPERTY(BlueprintReadOnly, Category = "Adventure")
    FGuid LastPlayedNode;

    bool IsCompleted(const FGuid& NodeId) const { return CompletedNodes.Contains(NodeId); }
    bool IsSealBroken(const FGuid& NodeId) const { return BrokenSeals.Contains(NodeId); }

    int32 GetShards(const FGameplayTag& Action) const
    {
        const int32* Found = Shards.Find(Action);
        return Found ? *Found : 0;
    }

    // The permanent uses of Action the shards give, through Table (0 with no
    // table)
    int32 GetPermanentUses(const FGameplayTag& Action, const UConnectIt_ActionShardTableDataAsset* Table) const;

    int32 GetItemCount(const FGameplayTag& Action) const
    {
        const int32* Found = ItemUses.Find(Action);
        return Found ? *Found : 0;
    }

    // --- The rules (the Adventure subsystem calls these, then saves) ---

    // Where Node stands: Completed if won; else Hidden unless the hub has
    // revealed it (bRevealed); else Gated if it still needs anything (OutCheck
    // says what), else Available
    EConnectItNodeState GetNodeState(const UConnectIt_MapNodeDataAsset& Node, bool bRevealed,
        const UConnectIt_ActionShardTableDataAsset* Table, FConnectItNodeRequirementCheck& OutCheck) const;

    // Whether Node's seal can be broken now: it has one, it isn't broken, and
    // the seals it needs are held. OutWhyNot says why not.
    bool CanBreakSeal(const UConnectIt_MapNodeDataAsset& Node, FString* OutWhyNot = nullptr) const;

    // Spends the seals Node's gate needs; its seal stays broken for good.
    // False (and no change) if CanBreakSeal fails.
    bool BreakSeal(const UConnectIt_MapNodeDataAsset& Node);

    // A match of Node ended. Win or lose, the inventory becomes the match's
    // (items spent are gone, items granted are kept) and Node is the last
    // played. On a first win, Node is completed and its rewards granted:
    // shards, item actions (each up to its cap in Items -- an action Items
    // doesn't list can't be held), seals. Returns whether rewards were
    // granted (never twice for a node).
    bool RecordResult(const UConnectIt_MapNodeDataAsset& Node, bool bWon,
        const TArray<FItemActionEntry>& Inventory, const UItemActionInventoryDataAsset* Items);

    // Completes Node and grants its rewards (items up to their caps) -- once:
    // false if it was already complete. The inventory is otherwise untouched
    // (a sub-map node completes this way when its map's Exit is won).
    bool CompleteNode(const UConnectIt_MapNodeDataAsset& Node, const UItemActionInventoryDataAsset* Items);

    // An open node's permanent actions, from Loadout (the level's Player
    // director's) and the shards: each loadout entry the shard table doesn't
    // price (Place Piece: everyone's) as it is; and each priced action the
    // shards give permanent uses of, with those uses as its match uses -- its
    // turn uses and cooldown from its loadout entry if it has one, else 1 and
    // 0. A priced action with no uses is left out.
    TArray<FPermanentActionConfig> BuildOpenPermanentActions(const UActionLoadoutDataAsset* Loadout,
        const UConnectIt_ActionShardTableDataAsset* Table) const;

    // The inventory as a match plays it: each held item action by class
    // (FindActionClass); one whose class can't be found is left out (logged)
    TArray<FItemActionEntry> MakeInventory(const UConnectIt_ActionShardTableDataAsset* Table,
        const UItemActionInventoryDataAsset* Items) const;

    // The class of the player action Action names (the save's key): the
    // shard table entry's Action Class, else the item action Items lists with
    // that tag. Null if neither has it.
    static TSubclassOf<UTurnBasedPlayerAction> FindActionClass(const FGameplayTag& Action,
        const UConnectIt_ActionShardTableDataAsset* Table, const UItemActionInventoryDataAsset* Items);
};

// What a node still needs before it can be started (UConnectIt_MapNodeDataAsset::
// CheckRequirements). Required actions are always checked; seals only until the
// node's seal is broken.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItNodeRequirementCheck
{
    GENERATED_BODY()

    // Required actions with no permanent use yet
    UPROPERTY(BlueprintReadOnly, Category = "Adventure")
    FGameplayTagContainer MissingActions;

    // Required specific seals not held
    UPROPERTY(BlueprintReadOnly, Category = "Adventure")
    FGameplayTagContainer MissingSeals;

    // General seals short
    UPROPERTY(BlueprintReadOnly, Category = "Adventure")
    int32 MissingGeneralSeals = 0;

    bool IsMet() const { return MissingActions.IsEmpty() && MissingSeals.IsEmpty() && MissingGeneralSeals == 0; }
    bool IsMissingSeals() const { return !MissingSeals.IsEmpty() || MissingGeneralSeals > 0; }
};
