// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
// #include "GameFramework/GameModeBase.h"
#include "TurnBasedGameMode.generated.h"

class UTurnBasedParticipantManagerComponent;

/**
 * 
 */
UCLASS()
class UNREALTURNBASEDMECHANICS_API ATurnBasedGameMode : public AGameMode
{
	GENERATED_BODY()

public:

	ATurnBasedGameMode();

	// --- Configuration ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn Based|Config")
	float TurnDuration = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn Based|Config")
	int32 ForfeitThreshold = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn Based|Config")
	float ReconnectTimeout = 30.f;

	// --- Overrides ---

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	// virtual void SwapPlayerControllers(APlayerController* OldPC, APlayerController* NewPC) override;
	virtual void HandleMatchHasStarted() override;

	// --- Game Setup ---

	// Called to register an AI controller as a participant
	// Call this before StartReadyCheck
	UFUNCTION(BlueprintCallable, Category = "Turn Based")
	void RegisterAIParticipant(
		AController* AIController,
		const FString& DisplayName = TEXT("AI")) const;

	// Signals that all participants have been registered
	// Transitions manager to WaitingForReady
	UFUNCTION(BlueprintCallable, Category = "Turn Based")
	void StartReadyCheck();

protected:

	UTurnBasedParticipantManagerComponent* GetParticipantManager() const;
	
};
