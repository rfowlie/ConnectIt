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

	// Loadout display name — for debugging and UI
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
	FString LoadoutName = TEXT("Unnamed Loadout");

	// Actions available to this participant
	// Each entry is an instanced UTurnBasedAction subclass
	// configured inline in the asset editor
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Loadout")
	TArray<TObjectPtr<UTurnBasedAction>> Actions;

	// Tags of actions banned in this specific level or context
	// Checked by the action component before allowing activation
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
	FGameplayTagContainer BannedActionTags;

	// Helper — returns only non-banned actions
	TArray<UTurnBasedAction*> GetPermittedActions() const
	{
		TArray<UTurnBasedAction*> Permitted;
		for (UTurnBasedAction* Action : Actions)
		{
			if (IsValid(Action) &&
				!BannedActionTags.HasTag(Action->ActionTag))
			{
				Permitted.Add(Action);
			}
		}
		return Permitted;
	}
};
