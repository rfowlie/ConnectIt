// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/Interpreter/BoardStateInterpreterBase.h"

#include "Board/BoardStateComponentBase.h"


void UBoardStateInterpreterBase::BindToBoardStateComponent(UBoardStateComponentBase* InComponent)
{
    if (!IsValid(InComponent))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("BoardStateInterpreterBase: %s — BindToBoardStateComponent "
                 "called with null component"),
            *GetName());
        return;
    }

    if (IsValid(BoundComponent))
    {
        BoundComponent->OnBoardStateChanged.RemoveDynamic(
            this, &UBoardStateInterpreterBase::HandleBoardStateChanged);

        UE_LOG(LogTemp, Log,
            TEXT("BoardStateInterpreterBase: %s unbound from previous component"),
            *GetName());
    }

    BoundComponent = InComponent;
    BoundComponent->OnBoardStateChanged.AddDynamic(
        this, &UBoardStateInterpreterBase::HandleBoardStateChanged);

    UE_LOG(LogTemp, Log,
        TEXT("BoardStateInterpreterBase: %s bound to %s"),
        *GetName(), *InComponent->GetName());
}

bool UBoardStateInterpreterBase::IsBound() const
{
    return IsValid(BoundComponent);
}

void UBoardStateInterpreterBase::HandleBoardStateChanged()
{
    if (!IsValid(BoundComponent)) return;
    OnBoardStateChanged(BoundComponent);
}

void UBoardStateInterpreterBase::OnBoardStateChanged_Implementation(
    const UBoardStateComponentBase* Component)
{
    // Base does nothing -- subclasses cast Component to their
    // project-specific type and read previous/current state from it.
}
