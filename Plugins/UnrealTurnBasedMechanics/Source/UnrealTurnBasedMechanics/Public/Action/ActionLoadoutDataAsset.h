// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "TurnBasedAction.h"
#include "ActionLoadoutDataAsset.generated.h"


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

    // --- Turn Actions ---
    // Instanced inline -- designer configures each entry directly
    // Accessed by component via GetPermittedActions()

    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly,
        Category = "Loadout|Turn Actions")
    TArray<TObjectPtr<UTurnBasedAction>> Actions;

    // Tags of actions banned in this level or context
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Loadout|Turn Actions")
    FGameplayTagContainer BannedActionTags;

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

    // --- Turn Action Accessors ---

    UFUNCTION(BlueprintPure, Category = "Loadout")
    TArray<UTurnBasedAction*> GetPermittedActions() const;

    UFUNCTION(BlueprintPure, Category = "Loadout")
    TArray<UTurnBasedAction*> GetRequiredActions() const;

    UFUNCTION(BlueprintPure, Category = "Loadout")
    TArray<UTurnBasedAction*> GetOptionalActions() const;

    UFUNCTION(BlueprintPure, Category = "Loadout")
    bool IsActionPermitted(FGameplayTag ActionTag) const;

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

    // Shared by GetRequiredActions/GetOptionalActions
    TArray<UTurnBasedAction*> FilterPermittedActionsByRequired(bool bRequired) const;
};
