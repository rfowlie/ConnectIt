// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Action/TurnBasedActionsComponent.h"
#include "ConnectIt_TurnBasedActionsComponent.generated.h"

// ConnectIt-specific actions component. The only override is
// CanAutoEndTurn_Implementation -- the base treats every bIsRequired action
// as mandatory (AND), which doesn't fit PlacePiece/SWAP: SWAP is meant to
// act as the entire turn, an *alternative* to PlacePiece, not an addition
// to it. RequiredActionTagA/B name that alternate pair -- turn can end once
// either one is complete, while any other bIsRequired action (none today,
// future-proofed) still needs its own completion, ANDed in exactly as the
// base does. Installed via AConnectIt_PlayerController's FObjectInitializer
// override of the base's hardcoded ActionsComponent subobject class -- no
// plugin file is touched.
UCLASS()
class CONNECTIT_API UConnectIt_TurnBasedActionsComponent : public UTurnBasedActionsComponent
{
    GENERATED_BODY()

public:

    // The two action tags treated as an OR'd pair for turn-end purposes
    // (PlacePiece's and SwapPiece's ActionTag). Defaults left invalid on
    // purpose -- see CanAutoEndTurn_Implementation's fallback when neither
    // is configured.
    UPROPERTY(EditDefaultsOnly, Category = "Turn Based|Behaviour")
    FGameplayTag RequiredActionTagA;

    UPROPERTY(EditDefaultsOnly, Category = "Turn Based|Behaviour")
    FGameplayTag RequiredActionTagB;

protected:

    virtual bool CanAutoEndTurn_Implementation() const override;
};
