// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InputTriggers.h"
#include "InputTagBinding.generated.h"

class UInputAction;

// One configured input trigger, dispatched by BindingTag through
// UInputTagBinder::OnInputTagTriggered -- adding a new bound input is
// purely data: pick an InputAction, name it with BindingTag, add one case
// wherever the consumer switches on BindingTag. No new C++ required per
// binding.
USTRUCT(BlueprintType)
struct UNREALGAMEMECHANICS_API FInputTagBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    FGameplayTag BindingTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> InputAction = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    ETriggerEvent TriggerEvent = ETriggerEvent::Triggered;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    FKey Key;
};

// TODO: figure out if we do not need to pass the tag but just use the FInputActionInstance to branch
// Fired by UInputTagBinder whenever any configured FInputTagBinding's
// InputAction triggers -- carries that entry's BindingTag so one shared
// handler can dispatch by tag instead of needing one bound function per
// input.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnInputTriggered, const FInputActionInstance&, Instance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnInputTagTriggered, const FInputActionInstance&, Instance, FGameplayTag, BindingTag);
