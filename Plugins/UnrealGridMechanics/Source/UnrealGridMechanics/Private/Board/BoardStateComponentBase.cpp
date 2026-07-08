// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/BoardStateComponentBase.h"
#include "Board/Interpreter/BoardStateInterpreter.h"


UBoardStateComponentBase::UBoardStateComponentBase()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UBoardStateComponentBase::RegisterInterpreter(
	UBoardStateInterpreter* Interpreter)
{
	if (!IsValid(Interpreter))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("BoardStateComponentBase: RegisterInterpreter called with null"));
		return;
	}

	Interpreters.AddUnique(Interpreter);

	UE_LOG(LogTemp, Log,
		TEXT("BoardStateComponentBase: Registered interpreter %s"),
		*Interpreter->GetName());
}

void UBoardStateComponentBase::BroadcastChange()
{
	OnBoardStateChanged.Broadcast();
}


