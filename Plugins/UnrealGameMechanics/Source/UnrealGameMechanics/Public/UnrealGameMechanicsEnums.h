// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UnrealGameMechanicsEnums.generated.h"


UENUM(BlueprintType)
enum class EGamePlayerType : uint8
{
	Player,
	Opponent,
	Observer
};