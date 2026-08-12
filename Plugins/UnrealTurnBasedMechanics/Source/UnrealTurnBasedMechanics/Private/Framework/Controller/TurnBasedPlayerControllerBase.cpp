// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Controller/TurnBasedPlayerControllerBase.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "Action/TurnBasedActionsComponent.h"
#include "Framework/Controller/TurnBasedControllerCoordinatorComponent.h"


ATurnBasedPlayerControllerBase::ATurnBasedPlayerControllerBase()
{
	ParticipantComponent =
		CreateDefaultSubobject<UTurnBasedParticipantComponent>(
			TEXT("ParticipantComponent"));

	ActionsComponent =
		CreateDefaultSubobject<UTurnBasedActionsComponent>(
			TEXT("ActionsComponent"));

	CoordinatorComponent =
		CreateDefaultSubobject<UTurnBasedControllerCoordinatorComponent>(
			TEXT("CoordinatorComponent"));
}

void ATurnBasedPlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();

	// Only the owning client confirms ready
	if (!IsLocalController()) return;

	if (bAutoNotifyReady && IsValid(ParticipantComponent))
	{
		ParticipantComponent->ServerNotifyReady();

		UE_LOG(LogTemp, Log,
			TEXT("TurnBasedPlayerControllerBase: %s notified ready"),
			*GetName());
	}
}