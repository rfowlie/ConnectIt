// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ConnectIt_ActionShardTableDataAsset.generated.h"


// One action's shard costs: how many more shards each permanent use takes --
// ShardsPerUse[0] for the 1st use, [1] the further shards for the 2nd... E.g.
// 3, 5, 8: the 1st use at 3 shards, the 2nd at 8 (3 + 5), the 3rd at 16.
// Designer-set, normally rising.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItActionShardCosts
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shards", meta = (ClampMin = 1))
    TArray<int32> ShardsPerUse;

    // How many permanent uses Shards has reached
    int32 GetPermanentUses(int32 Shards) const;
};

// How each action's shards become permanent uses in Adventure mode. A action is
// its action's tag (ConnectIt.PlayerAction.*). Permanent uses are permanent and come
// back every level; they are never saved, only the shards are, and derived
// through this table -- so retuning a table retunes every save.
//
// The project's table is named in Project Settings > Game > ConnectIt > Action
// Shard Table (Get). Place Piece needs no entry: it is everyone's from the
// start.
UCLASS(BlueprintType)
class CONNECTIT_API UConnectIt_ActionShardTableDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shards",
        meta = (Categories = "ConnectIt.PlayerAction", ForceInlineRow))
    TMap<FGameplayTag, FConnectItActionShardCosts> Actions;

    // How many permanent uses of Action Shards give (0 for a action with no
    // entry)
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure")
    int32 GetPermanentUses(FGameplayTag Action, int32 Shards) const;

    // The project's table (Project Settings), or null
    static const UConnectIt_ActionShardTableDataAsset* Get();

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
