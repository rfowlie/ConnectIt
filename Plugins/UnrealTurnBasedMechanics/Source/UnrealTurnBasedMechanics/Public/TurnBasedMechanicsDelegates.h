// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TurnBasedMechanicsStructs.h"


// NOTE: DYNAMIC delegates cannot be declared like this, they do not compile properly

// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnNotification,
// 	const FTurnNotification&, Notification);
//
// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOpponentTurnStarted,
// 	int32, ActiveParticipantSlotIndex);
//
// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnBasedActionEvent,
// 	UTurnBasedAction*, Action);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnNotification_Native,
	const FTurnNotification&);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnOpponentTurnStarted_Native,
	int32);