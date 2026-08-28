// Fill out your copyright notice in the Description page of Project Settings.

#include "State/StackedState.h"

// Base does nothing -- concrete subclasses override to bind/unbind whatever
// this state needs active while it's the top of the stack
void UStackedState::Initialize_Implementation()
{
}

void UStackedState::Deinitialize_Implementation()
{
}
