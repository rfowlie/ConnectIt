// Fill out your copyright notice in the Description page of Project Settings.

#include "Action/TurnBasedActionBase.h"
#include "UnrealTurnBasedMechanics.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"


void UTurnBasedActionBase::Activate(AController* InOwningController)
{
	if (bIsActive)
	{
		UE_LOG(LogTurnBasedMechanics, Warning,
			TEXT("TurnBasedActionBase: '%s' Activate called "
				 "but already active"),
			*ActionTag.ToString());
		return;
	}

	OwningController = InOwningController;
	bIsActive        = true;

	Activate_Internal();
	OnActivated.Broadcast(this);
	OnActivated_Native.Broadcast(this);

	UE_LOG(LogTurnBasedMechanics, Log,
		TEXT("TurnBasedActionBase: '%s' activated"),
		*ActionTag.ToString());
}

void UTurnBasedActionBase::ForceDeactivate()
{
	if (!bIsActive) return;

	bIsActive        = false;
	OwningController = nullptr;

	ForceDeactivate_Internal();
	OnForceDeactivated.Broadcast(this);
	OnForceDeactivated_Native.Broadcast(this);

	UE_LOG(LogTurnBasedMechanics, Log,
		TEXT("TurnBasedActionBase: '%s' force deactivated"),
		*ActionTag.ToString());
}

UWorld* UTurnBasedActionBase::GetWorld() const
{
	if (UActorComponent* Comp = GetTypedOuter<UActorComponent>())
		return Comp->GetWorld();
	if (AActor* Actor = GetTypedOuter<AActor>())
		return Actor->GetWorld();
	return nullptr;
}

APlayerController* UTurnBasedActionBase::GetPlayerController() const
{
	return Cast<APlayerController>(OwningController);
}

void UTurnBasedActionBase::Activate_Internal_Implementation()
{
}

void UTurnBasedActionBase::Deactivate_Internal_Implementation()
{
}

void UTurnBasedActionBase::ForceDeactivate_Internal_Implementation()
{
}
