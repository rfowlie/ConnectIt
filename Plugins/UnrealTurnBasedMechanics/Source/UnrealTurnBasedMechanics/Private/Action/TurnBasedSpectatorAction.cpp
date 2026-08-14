// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/TurnBasedSpectatorAction.h"
#include "GameFramework/Controller.h"



void UTurnBasedSpectatorAction::InitialiseAction(AController* InOwningController)
{
	OwningController = InOwningController;

	PostInitialiseAction();
}

void UTurnBasedSpectatorAction::Deactivate()
{
	// Viewer actions have no use tracking or completion state
	// Deactivate and ForceDeactivate are identical
	ForceDeactivate();
}