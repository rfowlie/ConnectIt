// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class ETurnBasedActionState : uint8
{
	Available    UMETA(DisplayName = "Available"),
	Active       UMETA(DisplayName = "Active"),
	Completed    UMETA(DisplayName = "Completed"),
	Cancelled    UMETA(DisplayName = "Cancelled"),
	OnCooldown   UMETA(DisplayName = "On Cooldown"),
	Exhausted    UMETA(DisplayName = "Exhausted")
};

// Type of modifier applied to a participant via the blackboard
UENUM(BlueprintType)
enum class ETurnModifierType : uint8
{
	ForcedMove          UMETA(DisplayName = "Forced Move"),
	ForcedMoveAdjacent  UMETA(DisplayName = "Forced Move Adjacent"),
	SkipTurn            UMETA(DisplayName = "Skip Turn"),
	RemovePlacedPiece   UMETA(DisplayName = "Remove Placed Piece"),
	Custom              UMETA(DisplayName = "Custom")
};