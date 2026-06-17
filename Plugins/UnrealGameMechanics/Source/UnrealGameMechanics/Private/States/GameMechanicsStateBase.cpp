// Fill out your copyright notice in the Description page of Project Settings.

#include "States/GameMechanicsStateBase.h"


void UGameMechanicsStateBase::Enter()
{
	if (OnGameStateEnter.IsBound()) { OnGameStateEnter.Broadcast(); }
	Enter_Internal_Implementation();
}

void UGameMechanicsStateBase::Exit()
{
	if (OnGameStateExit.IsBound()) { OnGameStateExit.Broadcast(); }
	Exit_Internal_Implementation();
}

void UGameMechanicsStateBase::Enter_Internal_Implementation()
{
}

void UGameMechanicsStateBase::Exit_Internal_Implementation()
{
}
