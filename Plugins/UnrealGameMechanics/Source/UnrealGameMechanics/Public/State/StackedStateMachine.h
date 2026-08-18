// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StackedStateMachine.generated.h"

class UStackedState;

// Generic stack-based state machine -- ported as-is from a pre-existing
// project's implementation (logic unchanged), renamed to be project-agnostic
// and placed in this plugin since nothing about the pattern is
// ConnectIt-specific. See UStackedState for the per-entry base class.
//
// Owns a stack of UStackedState instances. SetBaseState replaces the whole
// stack with one entry; PushState layers a new entry on top (optionally
// deinitializing what's underneath); PopState returns to whatever's below
// the current top. Any system with a "mini state machine within a larger
// flow" need owns one of these instead of hand-rolling an enum + switch --
// e.g. a multi-step action that walks a player through several distinct
// selection phases, one UStackedState subclass per phase.
UCLASS(Blueprintable, BlueprintType)
class UNREALGAMEMECHANICS_API UStackedStateMachine : public UObject
{
    GENERATED_BODY()

public:

    // Debug -- logs the current stack contents
    UFUNCTION(BlueprintCallable, Category = "State Machine")
    void PrintStateMachineInfo();

protected:

    UPROPERTY(BlueprintReadOnly, Category = "State Machine")
    TArray<TObjectPtr<UStackedState>> StateStack;

    void InitializeState(UStackedState* InState);
    void DeinitializeState(UStackedState* InState);
    void EmptyStack();

    UFUNCTION(BlueprintCallable, Category = "State Machine")
    UStackedState* PeakState();

    // Walk backwards through the stack deinitializing each state, then push
    // InState as the new (and only) entry
    UFUNCTION(BlueprintCallable, Category = "State Machine")
    void SetBaseState(UStackedState* InState, bool bDeinitialize);

    // Push a state onto the stack. bDeinitialize controls whether the state
    // currently on top gets deinitialized (fully replaced) or left running
    // underneath (e.g. paused rather than torn down) while InState is active
    UFUNCTION(BlueprintCallable, Category = "State Machine")
    void PushState(UStackedState* InState, bool bDeinitialize);

    // Return to whatever's below the current top of the stack
    UFUNCTION(BlueprintCallable, Category = "State Machine")
    void PopState();

    // TODO: not yet implemented -- see .cpp
    UFUNCTION(BlueprintCallable, Category = "State Machine")
    void SwapState(UStackedState* InState, bool bDoExit);
};
