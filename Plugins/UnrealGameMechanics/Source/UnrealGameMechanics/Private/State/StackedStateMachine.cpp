// Fill out your copyright notice in the Description page of Project Settings.

#include "State/StackedStateMachine.h"
#include "State/StackedState.h"

void UStackedStateMachine::PrintStateMachineInfo()
{
    UE_LOG(LogTemp, Display, TEXT("UStackedStateMachine::PrintStateMachineInfo"));
    UE_LOG(LogTemp, Display, TEXT("Num: %d"), StateStack.Num());

    for (int32 i = 0; i < StateStack.Num(); i++)
    {
        UE_LOG(LogTemp, Display, TEXT("State: %ls, Index: %d"),
            *StateStack[i]->GetClass()->GetName(), i);
    }
}

void UStackedStateMachine::EmptyStack()
{
    while (!StateStack.IsEmpty())
    {
        // Don't use PopState -- that would reinitialize whatever's
        // underneath. Here we only want every state on the stack
        // deinitialized, not the next one down reactivated.
        UStackedState* State = StateStack.Pop();
        State->Deinitialize();
    }
}

void UStackedStateMachine::InitializeState(UStackedState* InState)
{
    StateStack.Push(InState);
    InState->OnSetBaseState.AddUniqueDynamic(this, &ThisClass::SetBaseState);
    InState->OnPushState.AddUniqueDynamic(this, &ThisClass::PushState);
    InState->OnPopState.AddUniqueDynamic(this, &ThisClass::PopState);
    InState->Initialize();
}

void UStackedStateMachine::DeinitializeState(UStackedState* InState)
{
    if (InState == nullptr) { return; }
    InState->OnSetBaseState.RemoveDynamic(this, &ThisClass::SetBaseState);
    InState->OnPushState.RemoveDynamic(this, &ThisClass::PushState);
    InState->OnPopState.RemoveDynamic(this, &ThisClass::PopState);
    InState->Deinitialize();
}

UStackedState* UStackedStateMachine::PeakState()
{
    if (StateStack.IsEmpty()) { return nullptr; }
    return StateStack.Top();
}

void UStackedStateMachine::SetBaseState(UStackedState* InState)
{
    if (InState == nullptr) { return; }
    EmptyStack();
    PushState(InState, false);
}

void UStackedStateMachine::PushState(UStackedState* InState, bool bDeinitialize)
{
    if (InState == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("UStackedStateMachine::PushState: state is null!!"));
        return;
    }

    if (!StateStack.IsEmpty() && bDeinitialize)
    {
        DeinitializeState(StateStack.Top());
    }

    InitializeState(InState);
}

void UStackedStateMachine::PopState()
{
    if (StateStack.Num() <= 1)
    {
        UE_LOG(LogTemp, Error, TEXT(
            "UStackedStateMachine::PopState - Attempting to pop from the stack while only %d "
            "of states in the stack"), StateStack.Num());
        return;
    }

    // NOTE: calls Deinitialize() directly rather than DeinitializeState() --
    // matches the source implementation exactly, but means (unlike
    // SetBaseState/PushState-with-bDeinitialize) a popped state's
    // OnSetBaseState/OnPushState/OnPopState stay bound to this machine
    // instead of being unbound. Flagging rather than silently changing it,
    // per "keep it as is for now" -- worth revisiting.
    UStackedState* State = StateStack.Pop();
    State->Deinitialize();
    StateStack.Top()->Initialize();
}

void UStackedStateMachine::SwapState(UStackedState* InState, bool bDoExit)
{
    // TODO: pop current state then apply new state on top
    // call OnEnter on bottom state?
}
