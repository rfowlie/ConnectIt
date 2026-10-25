// Fill out your copyright notice in the Description page of Project Settings.

#include "Adventure/ConnectIt_AdventureProgress.h"
#include "Adventure/ConnectIt_ActionShardTable.h"

int32 FConnectItAdventureProgress::GetPermanentUses(const FGameplayTag& Action, const UConnectIt_ActionShardTable* Table) const
{
    return Table ? Table->GetPermanentUses(Action, GetShards(Action)) : 0;
}
