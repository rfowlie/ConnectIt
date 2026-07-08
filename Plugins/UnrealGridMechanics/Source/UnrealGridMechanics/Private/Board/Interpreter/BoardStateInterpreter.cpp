// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/Interpreter/BoardStateInterpreter.h"
#include "Board/BoardStateComponentBase.h"


UBoardStateInterpreter::UBoardStateInterpreter()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBoardStateInterpreter::BindToBoardStateComponent(
	UBoardStateComponentBase* InComponent)
{
	if (!IsValid(InComponent))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("BoardStateInterpreter: %s — BindToBoardStateComponent "
				 "called with null component"),
			*GetName());
		return;
	}

	// Unbind from previous if already bound
	if (IsValid(BoundComponent))
	{
		BoundComponent->OnBoardStateChanged.RemoveDynamic(
			this, &UBoardStateInterpreter::HandleBoardStateChanged);

		UE_LOG(LogTemp, Log,
			TEXT("BoardStateInterpreter: %s unbound from previous component"),
			*GetName());
	}

	BoundComponent = InComponent;
	BoundComponent->OnBoardStateChanged.AddDynamic(
		this, &UBoardStateInterpreter::HandleBoardStateChanged);

	UE_LOG(LogTemp, Log,
		TEXT("BoardStateInterpreter: %s bound to %s"),
		*GetName(), *InComponent->GetName());
}

bool UBoardStateInterpreter::IsBound() const
{
	return IsValid(BoundComponent);
}

void UBoardStateInterpreter::HandleBoardStateChanged()
{
	if (!IsValid(BoundComponent)) return;
	OnBoardStateChanged(BoundComponent);
}

void UBoardStateInterpreter::OnBoardStateChanged_Implementation(
	const UBoardStateComponentBase* Component)
{
	// Base does nothing
	// Subclasses cast Component to their project-specific type
	// and read previous/current state from it
}