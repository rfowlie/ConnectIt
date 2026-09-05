// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Controller/TurnBasedAIController.h"
#include "GridMechanicsBaseStructs.h"
#include "TurnBasedMechanicsStructs.h"
#include "ConnectIt_AIController.generated.h"


UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API AConnectIt_AIController : public ATurnBasedAIController
{
	GENERATED_BODY()

public:

	AConnectIt_AIController();

protected:

	virtual void BeginPlay() override;

	// Called when it is our turn and no forced move exists
	// Subclasses of this controller implement actual intelligence
	UFUNCTION(BlueprintImplementableEvent, Category = "ConnectIt|AI")
	void BeginMakeDecision();
	
private:

	void InitialiseFromLevelConfig();

	// Checks blackboard for forced move -- returns true if applied
	bool CheckAndApplyForcedMove();
	
};