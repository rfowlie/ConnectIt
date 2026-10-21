// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/ConnectIt_Director.h"
#include "ConnectIt_LevelDirector.generated.h"


// The level's own script: reacts to the match (the shared director events,
// anything it can reach through the world) and, being the level, is the one
// director that changes the board and the rules -- not a side's loadout
// (ask that side's director). One instance per match, owned by the GameMode;
// named by class on the match preset (none = no level script). Subclass it in
// Blueprint per adventure level.
//
// (Level triggers and Blueprint board / scoring-rule changes arrive in later
// stages of the three-directors plan.)
UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API UConnectIt_LevelDirector : public UConnectIt_Director
{
    GENERATED_BODY()
};
