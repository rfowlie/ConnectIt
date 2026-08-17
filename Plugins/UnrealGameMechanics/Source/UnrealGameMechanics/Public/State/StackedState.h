// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "StackedState.generated.h"

class UStackedState;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStackedStateSetBaseDelegate, UStackedState*, InState, bool, bDeinitialize);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStackedStatePushDelegate, UStackedState*, InState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStackedStatePopDelegate);

// Generic base for one entry on a UStackedStateMachine's stack. Ported from
// a pre-existing project's stack-based state pattern -- kept functionally
// as-is (see UStackedStateMachine for the stack logic itself), renamed to
// be project-agnostic and placed in this plugin since nothing about the
// pattern is ConnectIt-specific.
//
// Ported deliberately without the source project's IGigafireInputHandler
// interface -- that dependency wasn't provided and its contract is unknown,
// so it's not something this port can honestly reproduce. Concrete states
// bind whatever input they need themselves (Initialize/Deinitialize are the
// hook for that), the same way UTurnBasedAction's own selection binding
// works in the sibling UnrealTurnBasedMechanics plugin.
//
// Intended use: any action, controller, or system that has a "mini state
// machine within a larger flow" need -- e.g. a multi-step action that walks
// a player through several distinct selection phases -- owns a
// UStackedStateMachine and pushes/pops UStackedState subclasses to
// represent each phase, instead of hand-rolling an enum + a switch.
UCLASS(Abstract, Blueprintable, BlueprintType)
class UNREALGAMEMECHANICS_API UStackedState : public UObject
{
    GENERATED_BODY()

public:

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State Machine")
    FGameplayTag StateTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Machine")
    TObjectPtr<APlayerController> PlayerController;

    // Set up whatever this state needs active while it's the top of the stack
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "State Machine")
    void Initialize();

    // Undo whatever Initialize set up
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "State Machine")
    void Deinitialize();

    // --- Transition requests ---
    // The owning UStackedStateMachine binds to these three on push and
    // unbinds on pop -- a state fires one to request the machine change
    // the stack; the state itself never mutates the stack directly.

    UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "State Machine")
    FStackedStateSetBaseDelegate OnSetBaseState;

    UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "State Machine")
    FStackedStatePushDelegate OnPushState;

    UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "State Machine")
    FStackedStatePopDelegate OnPopState;
};
