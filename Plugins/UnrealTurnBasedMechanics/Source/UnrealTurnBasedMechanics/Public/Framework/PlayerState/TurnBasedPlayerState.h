// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TurnBasedMechanicsStructs.h"
#include "GameFramework/PlayerState.h"
#include "TurnBasedPlayerState.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerReadyChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerForfeited);

/**
 * 
 */
UCLASS()
class UNREALTURNBASEDMECHANICS_API ATurnBasedPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	ATurnBasedPlayerState();

	// How many turns this player has missed in a row
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_TurnsMissed, Category = "Turn Based")
	int32 TurnsMissed = 0;

	// Whether this player has forfeited the game
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Forfeited,	Category = "Turn Based")
	bool bForfeited = false;

	// Whether this player has confirmed ready to start
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Ready,	Category = "Turn Based")
	bool bIsReady = false;

	// Participant type — set by GameMode on registration
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Turn Based")
	EParticipantType ParticipantType = EParticipantType::Human;

	// --- Server only mutations ---

	void Server_IncrementTurnsMissed();
	void Server_SetForfeited();
	void Server_SetReady(bool bReady);

	// --- Delegates — fire on all clients via OnRep ---

	UPROPERTY(BlueprintAssignable, Category = "Turn Based")
	FOnPlayerReadyChanged OnReadyChanged;

	UPROPERTY(BlueprintAssignable, Category = "Turn Based")
	FOnPlayerForfeited OnForfeited;

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	UFUNCTION()
	void OnRep_TurnsMissed();

	UFUNCTION()
	void OnRep_Forfeited();

	UFUNCTION()
	void OnRep_Ready();
};
