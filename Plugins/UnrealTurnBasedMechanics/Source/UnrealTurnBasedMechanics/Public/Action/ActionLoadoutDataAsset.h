// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "TurnBasedAction.h"
#include "ActionLoadoutDataAsset.generated.h"


UCLASS(BlueprintType)
class UNREALTURNBASEDMECHANICS_API UActionLoadOutDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	// Display name -- for debugging and UI
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
	FString LoadoutName = TEXT("Unnamed Loadout");

	// Actions available to this participant
	// Each entry is an instanced UTurnBasedAction subclass
	// configured inline in the asset editor
	// Designer sets bIsRequired, bIsCancellable, cooldown etc. per entry
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Loadout")
	TArray<TObjectPtr<UTurnBasedAction>> Actions;

	// Tags of actions banned in this specific level or context
	// Checked by the action component before allowing activation
	// Allows the same loadout asset to be reused across levels
	// with specific actions disabled per level without duplicating the asset
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
	FGameplayTagContainer BannedActionTags;

	// Returns only actions whose tag is not in BannedActionTags
	// Called by UTurnBasedActionComponent during initialisation
	UFUNCTION(BlueprintPure, Category = "Loadout")
	TArray<UTurnBasedAction*> GetPermittedActions() const;

	// Returns only required actions from the permitted set
	UFUNCTION(BlueprintPure, Category = "Loadout")
	TArray<UTurnBasedAction*> GetRequiredActions() const;

	// Returns only optional actions from the permitted set
	UFUNCTION(BlueprintPure, Category = "Loadout")
	TArray<UTurnBasedAction*> GetOptionalActions() const;

	// Returns true if a specific action tag is permitted
	UFUNCTION(BlueprintPure, Category = "Loadout")
	bool IsActionPermitted(FGameplayTag ActionTag) const;

#if WITH_EDITOR
	// Validates the loadout in editor
	// Warns if required actions are missing or tags are duplicated
	virtual EDataValidationResult IsDataValid(
		FDataValidationContext& Context) const override;
#endif
	
};
