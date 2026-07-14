// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/TurnBasedViewerAction.h"
#include "GameFramework/Controller.h"


void UTurnBasedViewerAction::Activate(AController* InOwningController)
{
	if (bIsActive)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("TurnBasedViewerAction: '%s' Activate called "
				 "but already active"),
			*ActionTag.ToString());
		return;
	}

	OwningController = InOwningController;
	bIsActive        = true;

	OnActivated_Implementation();
	OnActionActivated.Broadcast(this);
	OnActivated_Native.Broadcast(this);

	UE_LOG(LogTemp, Log,
		TEXT("TurnBasedViewerAction: '%s' activated"),
		*ActionTag.ToString());
}

void UTurnBasedViewerAction::Deactivate()
{
	if (!bIsActive)
	{
		// Silently ignore -- safe to call when already inactive
		return;
	}

	bIsActive = false;

	OnDeactivated_Implementation();
	OnActionDeactivated.Broadcast(this);
	OnDeactivated_Native.Broadcast(this);

	OwningController = nullptr;

	UE_LOG(LogTemp, Log,
		TEXT("TurnBasedViewerAction: '%s' deactivated"),
		*ActionTag.ToString());
}

UWorld* UTurnBasedViewerAction::GetWorld() const
{
	if (UActorComponent* Comp = GetTypedOuter<UActorComponent>())
		return Comp->GetWorld();
	if (AActor* Actor = GetTypedOuter<AActor>())
		return Actor->GetWorld();
	return nullptr;
}

void UTurnBasedViewerAction::OnDeactivated_Implementation()
{
}

void UTurnBasedViewerAction::OnActivated_Implementation()
{
}