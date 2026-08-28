// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widget/DWidgetBase.h"
#include "GameplayTagContainer.h"
#include "DWidget_TurnBasedActionsComponent.generated.h"

class UTurnBasedActionsComponent;
struct FTurnActionSnapshot;
struct FTurnActionRequest;

// Tracks UTurnBasedActionsComponent specifically -- top/root action tags,
// stack depth, awaiting-confirmation state, pushed straight through to BP
// as each changes (see UDWidgetBase's own class comment for the
// push/GetInfo() convention). Binds OnAction*Safe (FTurnActionSnapshot,
// a copied tag) rather than the raw-pointer OnActionPushed/OnActionPopped/
// OnActionCompleted/OnActionCancelled -- those hand out live objects with
// BlueprintCallable Complete()/Cancel() on them, so nothing reachable from
// this widget can accidentally mutate live game state.
//
// Resolved via the owning player's ActionsComponent -- this class is
// per-controller, not a world/GameState singleton like some sibling widgets.
UCLASS(Abstract)
class UNREALTURNBASEDMECHANICS_API UDWidget_TurnBasedActionsComponent : public UDWidgetBase
{
    GENERATED_BODY()

public:

    // False until the owning player's UTurnBasedActionsComponent has
    // actually been resolved -- the one deliberately pull-only exception
    // to the push model, checked once rather than pushed.
    UFUNCTION(BlueprintPure, Category = "Debug")
    bool IsSourceValid() const { return ResolvedSource != nullptr; }

protected:

    virtual void BindDelegates() override;
    virtual void UnbindDelegates() override;

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Debug")
    void OnTopActionUpdated(FGameplayTag ActionTag);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Debug")
    void OnRootActionUpdated(FGameplayTag ActionTag);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Debug")
    void OnStackDepthUpdated(int32 NewStackDepth);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Debug")
    void OnAwaitingRequestConfirmationUpdated(bool bAwaiting);

private:

    // Shared by all four OnAction*Safe delegates and OnBoardChangeRequested
    // -- none of them map to a single field 1:1 (e.g. after a pop, the new
    // top is a DIFFERENT action than the one popped), so every one of them
    // re-derives the full current picture via GetInfo() rather than trying
    // to thread its own event's payload through piecemeal.
    UFUNCTION()
    void HandleActionStackChangedSafe(const FTurnActionSnapshot& Snapshot);

    UFUNCTION()
    void HandleBoardChangeRequested(const FTurnActionRequest& Request);

    void PushCurrentInfo();

    UPROPERTY()
    TObjectPtr<UTurnBasedActionsComponent> ResolvedSource = nullptr;
    
};
