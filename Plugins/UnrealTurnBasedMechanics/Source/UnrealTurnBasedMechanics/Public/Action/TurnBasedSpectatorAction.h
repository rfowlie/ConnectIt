// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "TurnBasedActionBase.h"
#include "TurnBasedSpectatorAction.generated.h"

class UTurnBasedSpectatorAction;

// Base class for passive controller actions that run when it
// is not this participant's turn
//
// Key differences from UTurnBasedAction:
//   -- Never completes naturally -- deactivated by external signal
//   -- Cannot request board state changes
//   -- Not queued or managed by required action system
//   -- Single instance per controller -- no cooldown or uses tracking
//   -- World access via typed outer (same pattern as UTurnBasedAction)
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class UNREALTURNBASEDMECHANICS_API UTurnBasedSpectatorAction : public UTurnBasedActionBase
{
    GENERATED_BODY()

public:

    // Explicit deactivate -- same as ForceDeactivate for viewer actions
    // Provided for clarity at call sites
    void Deactivate();
    
};
