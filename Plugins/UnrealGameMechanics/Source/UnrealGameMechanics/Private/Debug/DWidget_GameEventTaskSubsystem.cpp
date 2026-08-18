// Fill out your copyright notice in the Description page of Project Settings.

#include "Debug/DWidget_GameEventTaskSubsystem.h"
#include "GameEvent/GameEventTaskSubsystem.h"

void UDWidget_GameEventTaskSubsystem::BindDelegates()
{
    ResolvedSource = GetWorld()
        ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>()
        : nullptr;

    if (!IsValid(ResolvedSource)) return;

    ResolvedSource->OnActiveManagerTagsChanged.AddDynamic(
        this, &UDWidget_GameEventTaskSubsystem::HandleActiveManagerTagsChanged);
}

void UDWidget_GameEventTaskSubsystem::UnbindDelegates()
{
    if (!IsValid(ResolvedSource)) return;

    ResolvedSource->OnActiveManagerTagsChanged.RemoveDynamic(
        this, &UDWidget_GameEventTaskSubsystem::HandleActiveManagerTagsChanged);
}

void UDWidget_GameEventTaskSubsystem::RefreshFields()
{
    bSourceValid = IsValid(ResolvedSource);
    CachedActiveEventTags = bSourceValid
        ? ResolvedSource->GetTagsInQueue()
        : TArray<FGameplayTag>();
}

void UDWidget_GameEventTaskSubsystem::HandleActiveManagerTagsChanged()
{
    RefreshAll();
}
