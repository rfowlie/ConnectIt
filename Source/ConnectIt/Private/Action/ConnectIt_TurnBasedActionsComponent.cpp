// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ConnectIt_TurnBasedActionsComponent.h"

#include "Action/TurnBasedAction.h"


bool UConnectIt_TurnBasedActionsComponent::CanAutoEndTurn_Implementation() const
{
    // Neither tag configured -- nothing to OR together, fall back to the
    // base's plain AND-every-required-action behaviour entirely.
    if (!RequiredActionTagA.IsValid() && !RequiredActionTagB.IsValid())
    {
        return Super::CanAutoEndTurn_Implementation();
    }

    bool bAlternatePairSatisfied = false;

    for (UTurnBasedAction* Action : GetRequiredActions())
    {
        if (!IsValid(Action)) continue;

        // PlacePiece/SWAP are an alternate pair -- either one completing
        // satisfies the pair, and neither is individually mandatory below.
        if (Action->ActionTag == RequiredActionTagA ||
            Action->ActionTag == RequiredActionTagB)
        {
            if (Action->IsComplete())
            {
                bAlternatePairSatisfied = true;
            }
            continue;
        }

        // Any other required action (none today, future-proofed) is still
        // individually mandatory, exactly as the base class treats it.
        if (!Action->IsComplete())
        {
            return false;
        }
    }

    return bAlternatePairSatisfied;
}
