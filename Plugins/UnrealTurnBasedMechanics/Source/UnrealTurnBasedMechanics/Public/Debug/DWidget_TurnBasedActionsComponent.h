// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widget/DWidgetBase.h"
#include "GameplayTagContainer.h"
#include "DWidget_TurnBasedActionsComponent.generated.h"

class UTurnBasedActionsComponent;
class UTurnBasedActionBase;
class UTurnBasedAction;
struct FTurnActionRequest;

// Tracks UTurnBasedActionsComponent specifically -- top/root action stack
// state. Caches ActionTag only, never the live action pointer
// GetTopAction()/GetRootAction() return -- those objects have
// BlueprintCallable Complete()/Cancel() on them, so nothing reachable from
// this widget's cached fields can accidentally mutate live game state.
//
// Resolved via the owning player's ActionsComponent -- this class is
// per-controller, not a world/GameState singleton like some sibling widgets.
UCLASS(Abstract)
class UNREALTURNBASEDMECHANICS_API UDWidget_TurnBasedActionsComponent : public UDWidgetBase
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintPure, Category = "Debug")
    FGameplayTag GetTopActionTag() const { return CachedTopActionTag; }

    UFUNCTION(BlueprintPure, Category = "Debug")
    FGameplayTag GetRootActionTag() const { return CachedRootActionTag; }

    UFUNCTION(BlueprintPure, Category = "Debug")
    int32 GetStackDepth() const { return CachedStackDepth; }

    UFUNCTION(BlueprintPure, Category = "Debug")
    bool IsAwaitingRequestConfirmation() const { return bCachedAwaitingRequestConfirmation; }

    // False until the owning player's UTurnBasedActionsComponent has
    // actually been resolved
    UFUNCTION(BlueprintPure, Category = "Debug")
    bool IsSourceValid() const { return bSourceValid; }

protected:

    virtual void BindDelegates() override;
    virtual void UnbindDelegates() override;
    virtual void RefreshFields() override;

private:

    // Shared by OnActionPushed/OnActionPopped -- both are FOnActionBaseEvent
    UFUNCTION()
    void HandleActionStackChanged(UTurnBasedActionBase* Action);

    // Shared by OnActionCompleted/OnActionCancelled -- both are FOnTurnActionsEvent
    UFUNCTION()
    void HandleActionResolved(UTurnBasedAction* Action);

    UFUNCTION()
    void HandleBoardChangeRequested(const FTurnActionRequest& Request);

    UPROPERTY()
    TObjectPtr<UTurnBasedActionsComponent> ResolvedSource = nullptr;

    FGameplayTag CachedTopActionTag;
    FGameplayTag CachedRootActionTag;
    int32 CachedStackDepth = 0;
    bool bCachedAwaitingRequestConfirmation = false;
    bool bSourceValid = false;
};
