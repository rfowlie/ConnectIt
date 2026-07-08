// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GridMechanicsBaseStructs.h"
#include "TurnBasedMechanicsStructs.h"
#include "ConnectIt_AIController.generated.h"

class AConnectItBoardManager;
class UTurnBasedActionComponent;
class UTurnBasedParticipantComponent;
class UConnectItBlackboardSubsystem;
class UConnectIt_MinMaxTreeBuilder;


UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API AConnectIt_AIController : public AAIController
{
	GENERATED_BODY()

public:

	AConnectIt_AIController();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
		Category = "ConnectIt|Components")
	TObjectPtr<UTurnBasedParticipantComponent> ParticipantComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
		Category = "ConnectIt|Components")
	TObjectPtr<UTurnBasedActionComponent> ActionComponent = nullptr;


protected:

	virtual void BeginPlay() override;

	// Turn handlers
	UFUNCTION(BlueprintImplementableEvent, Category = "ConnectIt | Turn")
	void HandleTurnNotification(const FTurnNotification& Notification);


private:

	// Cached references
	UPROPERTY()
	TObjectPtr<AConnectItBoardManager> BoardActor = nullptr;

	int32 MySlotIndex = -1;

	void InitialiseFromBoardActor();
	

	// Checks blackboard for forced move modifier
	// Returns true if a forced move was found and submitted
	bool CheckAndApplyForcedMove();

	// Submits a move at the given position
	void SubmitMove(FGridPosition Position) const;
};
