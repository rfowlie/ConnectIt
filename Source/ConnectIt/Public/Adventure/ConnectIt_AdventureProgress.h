// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ConnectIt_AdventureProgress.generated.h"

class UConnectIt_SkillShardTable;


// A player's Adventure progress: what the save holds. Nodes are named by their
// data asset's NodeId (a GUID), so renaming or moving an asset keeps progress.
// Numbered uses aren't stored: they come from the shards through the skill
// shard table (GetNumberedUses).
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

    // Shards per skill (ConnectIt.Action.*)
    UPROPERTY(BlueprintReadOnly, Category = "Adventure")
    TMap<FGameplayTag, int32> Shards;

    // One-time uses per skill, from level rewards. Never count towards a
    // node's required skills.
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

    int32 GetShards(const FGameplayTag& Skill) const
    {
        const int32* Found = Shards.Find(Skill);
        return Found ? *Found : 0;
    }

    // The permanent uses of Skill the shards give, through Table (0 with no
    // table)
    int32 GetNumberedUses(const FGameplayTag& Skill, const UConnectIt_SkillShardTable* Table) const;
};

// What a node still needs before it can be started (UConnectIt_MapNodeDataAsset::
// CheckRequirements). Required skills are always checked; seals only until the
// node's seal is broken.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItNodeRequirementCheck
{
    GENERATED_BODY()

    // Required skills with no numbered use yet
    UPROPERTY(BlueprintReadOnly, Category = "Adventure")
    FGameplayTagContainer MissingSkills;

    // Required specific seals not held
    UPROPERTY(BlueprintReadOnly, Category = "Adventure")
    FGameplayTagContainer MissingSeals;

    // General seals short
    UPROPERTY(BlueprintReadOnly, Category = "Adventure")
    int32 MissingGeneralSeals = 0;

    bool IsMet() const { return MissingSkills.IsEmpty() && MissingSeals.IsEmpty() && MissingGeneralSeals == 0; }
    bool IsMissingSeals() const { return !MissingSeals.IsEmpty() || MissingGeneralSeals > 0; }
};
