// Fill out your copyright notice in the Description page of Project Settings.

// TurnBasedPlayerControllerBase.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interface/GridFactionInterface.h"
#include "TurnBasedPlayerControllerBase.generated.h"

class UTurnBasedParticipantComponent;
class UTurnBasedActionsComponent;
class UTurnBasedControllerCoordinatorComponent;

// Base player controller for turn-based participation
// Adds the three standard turn-based components
// PlayerState is created automatically by GameMode on PostLogin
// so no PlayerState handling is needed here
//
// All turn-to-action wiring is handled by the coordinator component
UCLASS(Abstract, Blueprintable, BlueprintType)
class UNREALTURNBASEDMECHANICS_API ATurnBasedPlayerControllerBase
	: public APlayerController, public IGridFactionInterface
{
	GENERATED_BODY()

public:

	ATurnBasedPlayerControllerBase();

	// TODO: make components protected
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
		Category = "Turn Based|Components")
	TObjectPtr<UTurnBasedParticipantComponent> ParticipantComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
		Category = "Turn Based|Components")
	TObjectPtr<UTurnBasedActionsComponent> ActionsComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
		Category = "Turn Based|Components")
	TObjectPtr<UTurnBasedControllerCoordinatorComponent>
		CoordinatorComponent = nullptr;

	// When true the controller calls ServerNotifyReady automatically
	// on BeginPlay for local controllers
	// Disable if a lobby or menu system handles ready confirmation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Turn Based|Config")
	bool bAutoNotifyReady = true;

	// --- Interface ---
	virtual int32 GetFactionId_Implementation() override;
	
protected:

	virtual void BeginPlay() override;
};