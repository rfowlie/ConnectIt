// Fill out your copyright notice in the Description page of Project Settings.

#include "Debug/DWidget_GameEventTaskSubsystem.h"
#include "GameEvent/GameEventTaskSubsystem.h"

void UDWidget_GameEventTaskSubsystem::BindDelegates()
{
    ResolvedSource = GetWorld()
        ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>()
        : nullptr;

    bSourceValid = IsValid(ResolvedSource);
    if (!bSourceValid) return;

    ResolvedSource->OnActiveManagerTagsChanged.AddDynamic(
        this, &UDWidget_GameEventTaskSubsystem::HandleActiveManagerTagsChanged);

    // Seed initial value through the same event future changes use.
    OnActiveEventTagsUpdated(ResolvedSource->GetTagsInQueue());
}

void UDWidget_GameEventTaskSubsystem::UnbindDelegates()
{
    if (!IsValid(ResolvedSource)) return;

    ResolvedSource->OnActiveManagerTagsChanged.RemoveDynamic(
        this, &UDWidget_GameEventTaskSubsystem::HandleActiveManagerTagsChanged);
}

void UDWidget_GameEventTaskSubsystem::HandleActiveManagerTagsChanged()
{
    if (!IsValid(ResolvedSource)) return;

    OnActiveEventTagsUpdated(ResolvedSource->GetTagsInQueue());
}
