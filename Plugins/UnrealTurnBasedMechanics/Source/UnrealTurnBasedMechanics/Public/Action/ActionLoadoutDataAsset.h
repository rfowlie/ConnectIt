// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "TurnBasedAction.h"
#include "Action/ActionConfig.h"
#include "Action/TurnEndRequirement.h"
#include "ActionLoadoutDataAsset.generated.h"


class UTurnBasedSpectatorAction;

UCLASS(BlueprintType)
class UNREALTURNBASEDMECHANICS_API UActionLoadoutDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

    // --- Display ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
    FString LoadoutName = TEXT("Unnamed Loadout");

    // --- System Action Classes ---
    // Vended as instances via getter functions
    // Component receives ownership via NewObject(Outer)

    // Active during your turn when no other action is running
    // Mandatory -- create a do-nothing action if your game has no idle state
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Loadout|System Actions")
    TSubclassOf<UTurnBasedAction> RootActionClass = nullptr;

    // Active immediately after your turn ends and during Updating phase
    // Appropriate for disabling input while resolution plays
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Loadout|System Actions")
    TSubclassOf<UTurnBasedSpectatorAction> IdleViewerActionClass = nullptr;

    // Active during opponent turn
    // Appropriate for observing board state, camera movement etc.
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Loadout|System Actions")
    TSubclassOf<UTurnBasedSpectatorAction> SpectatorViewerActionClass = nullptr;

    // Active during system pause
    // Pushed on top of stack -- stack preserved below
    // Framework provides UTurnBasedPauseAction as default
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Loadout|System Actions")
    TSubclassOf<UTurnBasedSpectatorAction> PauseViewerActionClass = nullptr;

    // Pushed while a board-change request this participant just sent is in
    // flight, blocking further stack mutation until the server answers.
    // Conceptually distinct from IdleViewerActionClass (turn already ended)
    // -- this is mid-turn, waiting on the participant's own pending request.
    // See UTurnBasedActionsComponent::NotifyBoardChangeOutcome.
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Loadout|System Actions")
    TSubclassOf<UTurnBasedSpectatorAction> AwaitingConfirmationActionClass = nullptr;

    // --- Turn Actions ---
    // The turn actions this loadout offers, split by lifecycle. An action
    // class belongs in exactly one of the two arrays. The actions component
    // builds one instance per entry (NewObject from the class), and each
    // player's PlayerState holds the live budget/cooldown state seeded from
    // these entries.

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Loadout|Turn Actions")
    TArray<FPermanentActionConfig> PermanentActions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Loadout|Turn Actions")
    TArray<FNumberedActionConfig> NumberedActions;

    // --- Turn End ---
    // Root of the requirement tree deciding when a turn can end (a Group or
    // Action Used node, nested as deep as needed). Left unset, no action
    // contributes to turn end: the turn can be ended at any time.

    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Loadout|Turn End")
    TObjectPtr<UTurnEndRequirementNode> TurnEndRequirements = nullptr;

    UFUNCTION(BlueprintPure, Category = "Loadout")
    bool HasTurnEndRequirements() const { return IsValid(TurnEndRequirements); }

    // --- System Action Vending ---
    // Each function creates a new instance owned by Outer
    // Returns null if the corresponding class is not set
    // Caller (action component) is responsible for validity check

    UFUNCTION(BlueprintCallable, Category = "Loadout")
    UTurnBasedAction* GetRootAction(UObject* Outer) const;

    UFUNCTION(BlueprintCallable, Category = "Loadout")
    UTurnBasedSpectatorAction* GetIdleViewerAction(UObject* Outer) const;

    UFUNCTION(BlueprintCallable, Category = "Loadout")
    UTurnBasedSpectatorAction* GetSpectatorAction(UObject* Outer) const;

    UFUNCTION(BlueprintCallable, Category = "Loadout")
    UTurnBasedSpectatorAction* GetPauseAction(UObject* Outer) const;

    UFUNCTION(BlueprintCallable, Category = "Loadout")
    UTurnBasedSpectatorAction* GetAwaitingConfirmationAction(UObject* Outer) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(
        FDataValidationContext& Context) const override;
#endif

private:

    // Shared by the Get*Action vending functions -- returns null if
    // Class is unset or Outer is invalid
    template <typename T>
    static T* CreateSystemAction(TSubclassOf<T> Class, UObject* Outer)
    {
        if (!Class || !IsValid(Outer)) return nullptr;
        return NewObject<T>(Outer, Class);
    }
};
