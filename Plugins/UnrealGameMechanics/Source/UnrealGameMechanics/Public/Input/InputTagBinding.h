// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InputTriggers.h"
#include "InputTagBinding.generated.h"

class UInputAction;


DECLARE_DYNAMIC_DELEGATE(FInputActionDelegate);
DECLARE_DYNAMIC_DELEGATE_OneParam(FInputActionInstanceDelegate, const FInputActionInstance&, Instance);

// One configured input trigger. Adding a new bound input is purely data:
// pick an InputAction, a TriggerEvent, a Key, then bind InputActionDelegate
// (e.g. via BindDynamic in ConstructInputBindings()) to whatever should run
// when it fires -- UInputTagBinder dispatches straight to that per-binding
// delegate, no shared switch/consumer needed.
USTRUCT(BlueprintType)
struct UNREALGAMEMECHANICS_API FInputTagBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TObjectPtr<UInputAction> InputAction = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    ETriggerEvent TriggerEvent = ETriggerEvent::Triggered;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    FKey Key;

    // the delegate to fire when this input is triggered
    UPROPERTY(BlueprintReadWrite, Category = "Input")
    FInputActionDelegate InputActionDelegate;

    // UPROPERTY(BlueprintReadWrite, Category = "Input")
    // FInputActionInstanceDelegate InputActionInstanceDelegate;
    
};


