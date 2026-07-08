// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TurnBasedMechanicsStructs.h"
#include "GameFramework/PlayerController.h"
#include "ConnectIt_PlayerController.generated.h"

class UTurnBasedParticipantComponent;
class UTurnBasedActionComponent;


/**
 * 
 */
UCLASS()
class CONNECTIT_API AConnectIt_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	AConnectIt_PlayerController();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
		Category = "ConnectIt|Components")
	TObjectPtr<UTurnBasedParticipantComponent> ParticipantComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
		Category = "ConnectIt|Components")
	TObjectPtr<UTurnBasedActionComponent> ActionComponent = nullptr;

protected:

	virtual void BeginPlay() override;

private:

	// Finds the board actor and initialises action component from loadout
	// Wires action component passthrough delegate to board manager
	void InitialiseFromBoardActor();

	// Delegate handlers
	UFUNCTION()
	void HandleTurnChanged(const FTurnNotification& Notification);

};

