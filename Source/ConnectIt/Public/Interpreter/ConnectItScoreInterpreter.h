// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/Interpreter/BoardStateInterpreter.h"
#include "ConnectItScoreInterpreter.generated.h"


// Reads score changes from board state and fires delegates for UI
// Runs on ALL clients independently
// UI widgets bind to the delegates and update their display
UCLASS(Blueprintable, ClassGroup=(ConnectIt),
	meta=(BlueprintSpawnableComponent))
class CONNECTIT_API UConnectItScoreInterpreter : public UBoardStateInterpreter
{
	GENERATED_BODY()

public:

	// Fired when any faction's score changes
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScoreUpdated,
		int32, FactionSlot, float, NewScore);

	UPROPERTY(BlueprintAssignable, Category = "Interpreter|Score")
	FOnScoreUpdated OnScoreUpdated;

	// Fired when game over state is detected
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameOverDetected,
		int32, WinningFactionSlot);

	UPROPERTY(BlueprintAssignable, Category = "Interpreter|Score")
	FOnGameOverDetected OnGameOverDetected;

	// Current scores -- readable by UI at any time
	UFUNCTION(BlueprintPure, Category = "Interpreter|Score")
	float GetFactionScore(int32 FactionSlot) const;

	UFUNCTION(BlueprintPure, Category = "Interpreter|Score")
	TArray<float> GetAllScores() const { return CachedScores; }

protected:

	virtual void OnBoardStateChanged_Implementation(
		const UBoardStateComponentBase* Component) override;

private:

	// Cached scores -- updated each board state change
	UPROPERTY()
	TArray<float> CachedScores;

	bool bGameOverFired = false;
	
};
