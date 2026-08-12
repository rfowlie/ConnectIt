// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
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


};
