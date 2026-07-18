// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/Spectator/TurnBasedPauseAction.h"
#include "UnrealTurnBasedMechanics.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"



void UTurnBasedPauseAction::Activate_Internal_Implementation()
{
	DisableInput();
	ShowPauseWidget();
}

void UTurnBasedPauseAction::Deactivate_Internal_Implementation()
{
	HidePauseWidget();
	EnableInput();
}

void UTurnBasedPauseAction::ShowPauseWidget()
{
	APlayerController* PC = GetPlayerController();
	if (!IsValid(PC)) return;
	if (!PauseWidgetClass) return;

	ActivePauseWidget = CreateWidget<UUserWidget>(PC, PauseWidgetClass);
	if (IsValid(ActivePauseWidget))
	{
		ActivePauseWidget->AddToViewport();

		UE_LOG(LogTurnBasedMechanics, Log,
			TEXT("TurnBasedPauseAction: "
				 "Pause widget added to viewport"));
	}
}

void UTurnBasedPauseAction::HidePauseWidget()
{
	if (IsValid(ActivePauseWidget))
	{
		ActivePauseWidget->RemoveFromParent();
		ActivePauseWidget = nullptr;

		UE_LOG(LogTurnBasedMechanics, Log,
			TEXT("TurnBasedPauseAction: "
				 "Pause widget removed from viewport"));
	}
}

void UTurnBasedPauseAction::DisableInput()
{
	APlayerController* PC = GetPlayerController();
	if (!IsValid(PC)) return;

	PC->DisableInput(PC);

	UE_LOG(LogTurnBasedMechanics, Log,
		TEXT("TurnBasedPauseAction: Input disabled"));
}

void UTurnBasedPauseAction::EnableInput()
{
	APlayerController* PC = GetPlayerController();
	if (!IsValid(PC)) return;

	PC->EnableInput(PC);

	UE_LOG(LogTurnBasedMechanics, Log,
		TEXT("TurnBasedPauseAction: Input enabled"));
}