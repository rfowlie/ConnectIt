// Fill out your copyright notice in the Description page of Project Settings.

#include "Debug/DWidget_GameEventTaskSubsystem.h"
#include "GameEvent/GameEventTaskSubsystem.h"

void UDWidget_GameEventTaskSubsystem::BindDelegates()
{
    if (const auto Subsystem = GetWorld()->GetSubsystem<UGameEventTaskSubsystem>())
    {
        Subsystem->OnActiveManagerTagsChanged.AddDynamic(
        this, &UDWidget_GameEventTaskSubsystem::HandleActiveManagerTagsChanged);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT(
            "UDWidget_GameEventTaskSubsystem::BindDelegates - could not bind to subsystem!"))
    }
}

void UDWidget_GameEventTaskSubsystem::UnbindDelegates()
{
    if (const auto Subsystem = GetWorld()->GetSubsystem<UGameEventTaskSubsystem>())
    {
        Subsystem->OnActiveManagerTagsChanged.RemoveDynamic(
        this, &UDWidget_GameEventTaskSubsystem::HandleActiveManagerTagsChanged);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT(
            "UDWidget_GameEventTaskSubsystem::BindDelegates - could not unbind from subsystem!"))
    }
}

void UDWidget_GameEventTaskSubsystem::HandleActiveManagerTagsChanged(const FGameplayTagContainer& TagContainer)
{
    OnActiveEventTagsUpdated(TagContainer);
}
