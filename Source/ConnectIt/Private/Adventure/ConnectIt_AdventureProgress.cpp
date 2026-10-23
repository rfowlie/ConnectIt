// Fill out your copyright notice in the Description page of Project Settings.

#include "Adventure/ConnectIt_AdventureProgress.h"
#include "Adventure/ConnectIt_SkillShardTable.h"

int32 FConnectItAdventureProgress::GetNumberedUses(const FGameplayTag& Skill, const UConnectIt_SkillShardTable* Table) const
{
    return Table ? Table->GetNumberedUses(Skill, GetShards(Skill)) : 0;
}
