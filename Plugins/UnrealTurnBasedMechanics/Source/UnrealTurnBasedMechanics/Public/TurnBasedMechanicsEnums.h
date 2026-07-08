// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


// Phase of the turn state machine
UENUM(BlueprintType)
enum class ETurnPhase : uint8
{
	WaitingForParticipants  UMETA(DisplayName = "Waiting For Participants"),
	WaitingForReady         UMETA(DisplayName = "Waiting For Ready"),
	TurnStart               UMETA(DisplayName = "Turn Start"),
	TurnActive              UMETA(DisplayName = "Turn Active"),
	TurnEnd                 UMETA(DisplayName = "Turn End"),
	TurnResolution          UMETA(DisplayName = "Turn Resolution"),
	TurnTimeout             UMETA(DisplayName = "Turn Timeout"),
	TurnSkipped             UMETA(DisplayName = "Turn Skipped"),
	Paused                  UMETA(DisplayName = "Paused"),
	GameOver                UMETA(DisplayName = "Game Over")
};

// Why a turn ended — passed to listeners for context
UENUM(BlueprintType)
enum class ETurnEndReason : uint8
{
	ParticipantEnded    UMETA(DisplayName = "Participant Ended Turn"),
	TimedOut            UMETA(DisplayName = "Timed Out"),
	Skipped             UMETA(DisplayName = "Skipped"),
	Forfeited           UMETA(DisplayName = "Forfeited"),
	GameOver            UMETA(DisplayName = "Game Over")
};

// Whether a participant is human or AI
UENUM(BlueprintType)
enum class EParticipantType : uint8
{
	Human     UMETA(DisplayName = "Human"),
	AI        UMETA(DisplayName = "AI"),
	Observer  UMETA(DisplayName = "Observer")
};

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