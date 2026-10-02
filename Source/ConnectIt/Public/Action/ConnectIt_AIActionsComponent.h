// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action/TurnBasedActionsComponent.h"
#include "ConnectIt_AIActionsComponent.generated.h"

// ActionsComponent subclass installed on AConnectIt_AIController, mirroring
// UConnectIt_TurnBasedActionsComponent's role on the player controller (see
// that class's own header comment).
//
// The base component has no bindable "my turn started" delegate -- only the
// protected OnTurnStarted BlueprintNativeEvent, meant for a component
// subclass to override. This is that subclass: once the base's default
// turn-start handling has run (stack reset to RootAction, matching a human's
// own turn start), it notifies the owning AConnectIt_AIController so it can
// begin deciding -- the AI-side equivalent of a human's turn-start UI
// becoming interactable.
UCLASS()
class CONNECTIT_API UConnectIt_AIActionsComponent : public UTurnBasedActionsComponent
{
    GENERATED_BODY()

protected:

    virtual void OnTurnStarted_Implementation(const FTurnStartContext& Context) override;
};
