// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action/TurnBasedSpectatorAction.h"
#include "TurnBasedPauseAction.generated.h"

class UUserWidget;


// Default pause viewer action -- lives in framework
// Projects set PauseWidgetClass to their own pause screen
// Disables all input on owning controller when active
// Restores input and removes widget on deactivation
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class UNREALTURNBASEDMECHANICS_API UTurnBasedPauseAction : public UTurnBasedSpectatorAction
{
	GENERATED_BODY()

public:

	// Widget class to spawn when paused
	// Set in Blueprint subclass or DataAsset per project
	UPROPERTY(EditAnywhere, BlueprintReadOnly,
		Category = "Pause|Config")
	TSubclassOf<UUserWidget> PauseWidgetClass = nullptr;

protected:

	virtual void Activate_Internal_Implementation() override;
	virtual void Deactivate_Internal_Implementation() override;

private:

	UPROPERTY()
	TObjectPtr<UUserWidget> ActivePauseWidget = nullptr;

	void ShowPauseWidget();
	void HidePauseWidget();
	void DisableInput();
	void EnableInput();
};