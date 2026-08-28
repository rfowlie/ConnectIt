// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action/TurnBasedSpectatorAction.h"
#include "ConnectIt_DefaultViewerAction.generated.h"

class UConnectIt_BoardStateComponent;

// Minimal viewer action for ConnectIt
// Enables cursor visibility while opponent acts
// Listens to board state changes to drive any observation UI
// Cannot affect board state
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CONNECTIT_API UConnectIt_DefaultViewerAction : public UTurnBasedSpectatorAction
{
	GENERATED_BODY()

protected:
	virtual void Activate_Internal_Implementation() override;
	virtual void Deactivate_Internal_Implementation() override;

	// Resolves and caches CachedBoardState via OwningController -- called
	// once by UTurnBasedSpectatorAction::InitialiseAction, well before this
	// action is ever activated. Eager, fail-loud resolution instead of
	// re-resolving on every single Activate/Deactivate (Bind/UnbindFromBoardState
	// used to call the world-context lookup every time this action's turn
	// came around, which for a spectator action can be frequent).
	virtual void PostInitialiseAction_Implementation() override;

private:

    // Binds to board state changes so UI can react to opponent moves
    void BindToBoardState();
    void UnbindFromBoardState();

    UFUNCTION()
    void HandleBoardStateChanged();

    UPROPERTY()
    TObjectPtr<UConnectIt_BoardStateComponent> CachedBoardState = nullptr;
};
