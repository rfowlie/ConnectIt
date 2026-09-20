// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action/TurnBasedActionsComponent.h"
#include "ConnectIt_TurnBasedActionsComponent.generated.h"

// ConnectIt's actions component. It used to override CanAutoEndTurn with a
// hardcoded PlacePiece/SWAP OR-pair (RequiredActionTagA/B); turn end is now
// data-driven by the loadout's TurnEndRequirements tree in the base
// component, so this class carries no behaviour of its own.
//
// Kept as an empty subclass on purpose: AConnectIt_PlayerController installs
// it via its FObjectInitializer override and Blueprint assets reference it,
// so removing the class would break them. It can be deleted (and the
// controller's override with it) once nothing references it.
UCLASS()
class CONNECTIT_API UConnectIt_TurnBasedActionsComponent : public UTurnBasedActionsComponent
{
    GENERATED_BODY()
};
