// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Adventure/ConnectIt_AdventureProgress.h"
#include "ConnectIt_MapNodeDataAsset.generated.h"

class UConnectIt_MatchPreset;
class UConnectIt_SkillShardTable;
class UTexture2D;
class UWorld;


// Which skills a node's level plays with
UENUM(BlueprintType)
enum class EConnectItSkillDesign : uint8
{
    // Place Piece, the player's numbered uses, and any itemed uses they bring
    Open,
    // The level's own loadout (its preset's Player director), whatever the
    // player has unlocked -- teaching levels, hard puzzle levels
    Closed,
};

// What a node needs before it can be started. Hand-set by the designer.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItNodeGate
{
    GENERATED_BODY()

    // Skills the player must have at least one numbered use of (itemed uses
    // don't count). Place Piece is everyone's.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gate", meta = (Categories = "ConnectIt.PlayerAction"))
    FGameplayTagContainer RequiredSkills;

    // Specific seals spent to open it, one of each
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gate", meta = (Categories = "ConnectIt.Adventure.Seal"))
    FGameplayTagContainer RequiredSeals;

    // General seals spent to open it
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gate", meta = (ClampMin = 0))
    int32 GeneralSeals = 0;

    bool HasSeals() const { return !RequiredSeals.IsEmpty() || GeneralSeals > 0; }
};

// What winning a node gives
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItNodeRewards
{
    GENERATED_BODY()

    // Shards per skill
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards",
        meta = (Categories = "ConnectIt.PlayerAction", ForceInlineRow))
    TMap<FGameplayTag, int32> Shards;

    // One-time uses per skill
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards",
        meta = (Categories = "ConnectIt.PlayerAction", ForceInlineRow))
    TMap<FGameplayTag, int32> ItemUses;

    // Specific seals (a region capstone's, for a known gate)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards", meta = (Categories = "ConnectIt.Adventure.Seal"))
    FGameplayTagContainer Seals;

    // General seals (optional hard / puzzle levels)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards", meta = (ClampMin = 0))
    int32 GeneralSeals = 0;
};

// One node of the Adventure hub: everything about it, including which nodes
// winning it reveals. A map node actor placed in a hub map holds one.
//
// A node plays a level (Level + Match Preset) or is a whole sub-map (another
// hub map: the node is complete when that map's exit node is) -- one or the
// other. Its NodeId names it in the save, so renaming or moving the asset
// keeps progress; a copy of the asset gets a new one.
UCLASS(BlueprintType)
class CONNECTIT_API UConnectIt_MapNodeDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:

    // --- Identity ---

    // Set when the asset is created (and anew for a copy). Never edit it: the
    // save refers to the node by it.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Node")
    FGuid NodeId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node")
    TSoftObjectPtr<UTexture2D> Thumbnail;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node", meta = (Categories = "ConnectIt.Adventure.Region"))
    FGameplayTag Region;

    // --- What it plays ---

    // The level to play (with Match Preset). Leave empty for a sub-map node.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Play")
    TSoftObjectPtr<UWorld> Level;

    // How Level is played; unset = the level's own Default Match Preset. Soft:
    // loaded when the node is started, not with the hub.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Play")
    TSoftObjectPtr<UConnectIt_MatchPreset> MatchPreset;

    // Another hub map this node stands for (a region); complete when that
    // map's exit node is. Leave empty for a level node.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Play")
    TSoftObjectPtr<UWorld> SubMap;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Play")
    EConnectItSkillDesign SkillDesign = EConnectItSkillDesign::Open;

    // --- The map ---

    // The nodes winning this one reveals (a gated one is revealed, but needs
    // its gate met before it can be started). Soft: loading a node doesn't
    // load the graph behind it.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
    TArray<TSoftObjectPtr<UConnectIt_MapNodeDataAsset>> Unlocks;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
    FConnectItNodeGate Gate;

    // --- Information (for the player and the map validator; never gates) ---

    // Skills that make the level much easier
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Information", meta = (Categories = "ConnectIt.PlayerAction"))
    FGameplayTagContainer RecommendedSkills;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Information", meta = (Categories = "ConnectIt.Adventure.Concept"))
    FGameplayTagContainer TeachesConcepts;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Information", meta = (Categories = "ConnectIt.Adventure.Concept"))
    FGameplayTagContainer UsesConcepts;

    // --- Rewards ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards")
    FConnectItNodeRewards Rewards;

    // ---

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure")
    bool IsSubMap() const { return !SubMap.IsNull(); }

    // What this node still needs for Progress: required skills without a
    // numbered use (through Table), and -- unless its seal is broken -- the
    // seals not held
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure")
    FConnectItNodeRequirementCheck CheckRequirements(
        const FConnectItAdventureProgress& Progress, const UConnectIt_SkillShardTable* Table) const;

    virtual void PostInitProperties() override;
    virtual void PostDuplicate(bool bDuplicateForPIE) override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
