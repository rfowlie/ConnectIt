// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GridMechanicsBaseStructs.h"
#include "TurnBasedMechanicsStructs.h"
#include "ConnectIt_AIController.generated.h"

class AConnectItBoardActor;
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

	// MinMax tree builder -- runs async on background thread
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
		Category = "ConnectIt|AI")
	TObjectPtr<UConnectIt_MinMaxTreeBuilder> TreeBuilder = nullptr;

protected:

	virtual void BeginPlay() override;

private:

	// Cached references
	UPROPERTY()
	TObjectPtr<AConnectItBoardActor> BoardActor = nullptr;

	int32 MySlotIndex = -1;

	void InitialiseFromBoardActor();

	// Turn handlers
	UFUNCTION()
	void HandleTurnStarted(const FTurnNotification& Notification);

	UFUNCTION()
	void HandleTurnEnded(const FTurnNotification& Notification);

	// Called when MinMax tree is built and solved
	UFUNCTION()
	void HandleMinMaxComplete();

	// Checks blackboard for forced move modifier
	// Returns true if a forced move was found and submitted
	bool CheckAndApplyForcedMove();

	// Submits a move at the given position
	void SubmitMove(FGridPosition Position) const;
};
