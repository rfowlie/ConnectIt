// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ConnectIt_SkillShardTable.generated.h"


// One skill's shard costs: how many more shards each numbered use takes --
// ShardsPerUse[0] for the 1st use, [1] the further shards for the 2nd... E.g.
// 3, 5, 8: the 1st use at 3 shards, the 2nd at 8 (3 + 5), the 3rd at 16.
// Designer-set, normally rising.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItSkillShardCosts
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shards", meta = (ClampMin = 1))
    TArray<int32> ShardsPerUse;

    // How many numbered uses Shards has reached
    int32 GetNumberedUses(int32 Shards) const;
};

// How each skill's shards become numbered uses in Adventure mode. A skill is
// its action's tag (ConnectIt.Action.*). Numbered uses are permanent and come
// back every level; they are never saved, only the shards are, and derived
// through this table -- so retuning a table retunes every save.
//
// The project's table is named in Project Settings > Game > ConnectIt > Skill
// Shard Table (Get). Place Piece needs no entry: it is everyone's from the
// start.
UCLASS(BlueprintType)
class CONNECTIT_API UConnectIt_SkillShardTable : public UDataAsset
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shards",
        meta = (Categories = "ConnectIt.Action", ForceInlineRow))
    TMap<FGameplayTag, FConnectItSkillShardCosts> Skills;

    // How many numbered uses of Skill Shards give (0 for a skill with no
    // entry)
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure")
    int32 GetNumberedUses(FGameplayTag Skill, int32 Shards) const;

    // The project's table (Project Settings), or null
    static const UConnectIt_SkillShardTable* Get();

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
