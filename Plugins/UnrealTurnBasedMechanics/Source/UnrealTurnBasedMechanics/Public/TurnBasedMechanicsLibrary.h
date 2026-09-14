// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TurnBasedMechanicsStructs.h"
#include "TurnBasedMechanicsLibrary.generated.h"

class UTurnBasedParticipantComponent;
/**
 *
 */
UCLASS()
class UNREALTURNBASEDMECHANICS_API UTurnBasedMechanicsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn Based")
	static bool IsLocalPlayerActiveParticipant(const UObject* WorldContextObject);

	// Display name for a participant row. USTRUCT member functions cannot
	// be UFUNCTIONs, so FTurnParticipantInfo::GetDisplayName() delegates to
	// this rather than being callable from Blueprint in place -- one
	// implementation either way.
	UFUNCTION(BlueprintPure, Category = "Turn Based|Participant",
		meta = (DisplayName = "Get Participant Display Name"))
	static FString GetParticipantDisplayName(
		const FTurnParticipantInfo& Participant,
		const FString& FallbackName = TEXT("Unknown"));

};
