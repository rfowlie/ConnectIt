// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GridMechanicsBaseEnums.generated.h"


UENUM(BlueprintType)
enum class EGridDirection : uint8
{
	Up    UMETA(DisplayName = "Up"),
	UpRight    UMETA(DisplayName = "Up Right"),
	Right UMETA(DisplayName = "Right"),
	DownRight UMETA(DisplayName = "Down Right"),
	Down  UMETA(DisplayName = "Down"),
	DownLeft  UMETA(DisplayName = "Down Left"),
	Left  UMETA(DisplayName = "Left"),
	UpLeft  UMETA(DisplayName = "Up Left"),

	Max   UMETA(Hidden)
};

FORCEINLINE EGridDirection operator+(EGridDirection A, EGridDirection B)
{
	constexpr int32 NumDirections = static_cast<int32>(EGridDirection::Max);
	const int32 Result = (static_cast<int32>(A) + static_cast<int32>(B)) % NumDirections;
	return static_cast<EGridDirection>(Result);
}

FORCEINLINE EGridDirection operator+(EGridDirection Direction, const int32 Offset)
{
	constexpr int32 NumDirections = static_cast<int32>(EGridDirection::Max);
	int32 Result = (static_cast<int32>(Direction) + Offset) % NumDirections;
	if (Result < 0) { Result += NumDirections; }
	return static_cast<EGridDirection>(Result);
}

FORCEINLINE EGridDirection operator-(EGridDirection A, EGridDirection B)
{
	constexpr int32 NumDirections = static_cast<int32>(EGridDirection::Max);
	const int32 Result = ((static_cast<int32>(A) - static_cast<int32>(B)) + NumDirections) % NumDirections;
	return static_cast<EGridDirection>(Result);
}

FORCEINLINE EGridDirection operator-=(EGridDirection Direction, const int32 Offset)
{
	constexpr int32 NumDirections = static_cast<int32>(EGridDirection::Max);
	int32 Result = ((static_cast<int32>(Direction) - Offset) + NumDirections) % NumDirections;
	if (Result < 0) { Result += NumDirections; }
	return static_cast<EGridDirection>(Result);
}