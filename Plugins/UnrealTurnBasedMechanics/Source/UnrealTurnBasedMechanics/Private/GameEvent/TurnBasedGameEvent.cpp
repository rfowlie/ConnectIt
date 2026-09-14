// Fill out your copyright notice in the Description page of Project Settings.


#include "GameEvent/TurnBasedGameEvent.h"


void UTurnBasedGameEvent::Execute_Implementation()
{
    // Base does nothing -- a concrete subclass that forgets to override
    // this would otherwise never report completion, so this deliberately
    // is not a safe no-op default beyond compiling; every real subclass
    // must override.
}

void UTurnBasedGameEvent::Complete()
{
    OnComplete.Broadcast(this);
}
