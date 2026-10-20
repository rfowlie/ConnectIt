// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "ConnectIt_PlayerState.generated.h"

// ConnectIt-specific player state. It used to hold the SWAP use budget
// (SwapUsesRemaining and its rep-notify/delegate); that is now a generic
// NumberedActions entry in the loadout, with its live state on the base
// ATurnBasedPlayerState (see GetNumberedActionState / CanUseAction /
// OnActionRuntimeStateUpdated). Nothing game-specific lives here yet.
//
// Kept as the game's PlayerState class (AConnectIt_GameMode and Blueprints
// reference it) so game-specific replicated state has somewhere to go.
UCLASS()
class CONNECTIT_API AConnectIt_PlayerState : public ATurnBasedPlayerState
{
    GENERATED_BODY()

public:
    AConnectIt_PlayerState();
    
};
