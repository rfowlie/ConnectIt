// Fill out your copyright notice in the Description page of Project Settings.

#include "Debug/DWidget_TurnBasedParticipantManagerComponent.h"

#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"

void UDWidget_TurnBasedParticipantManagerComponent::BindDelegates()
{
    APlayerController* OwningPC = GetOwningPlayer();
    if (!IsValid(OwningPC)) return;

    ResolvedSource = OwningPC->FindComponentByClass<UTurnBasedParticipantManagerComponent>();
    if (!IsValid(ResolvedSource)) return;

    ResolvedSource->OnTurnPhaseChanged.AddDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleTurnPhaseChanged);
    ResolvedSource->OnActiveControllerChanged.AddDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleActiveControllerChanged);
    ResolvedSource->OnParticipantForfeited.AddDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleParticipantForfeited);
    ResolvedSource->OnAllParticipantsReady.AddDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleAllParticipantsReady);
    ResolvedSource->OnGameOver.AddDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleGameOver);
}

void UDWidget_TurnBasedParticipantManagerComponent::UnbindDelegates()
{
    if (!IsValid(ResolvedSource)) return;

    ResolvedSource->OnTurnPhaseChanged.RemoveDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleTurnPhaseChanged);
    ResolvedSource->OnActiveControllerChanged.RemoveDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleActiveControllerChanged);
    ResolvedSource->OnParticipantForfeited.RemoveDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleParticipantForfeited);
    ResolvedSource->OnAllParticipantsReady.RemoveDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleAllParticipantsReady);
    ResolvedSource->OnGameOver.RemoveDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleGameOver);
}

void UDWidget_TurnBasedParticipantManagerComponent::RefreshFields()
{
    bSourceValid = IsValid(ResolvedSource);
    if (!bSourceValid) return;

    CachedTurnPhase = ResolvedSource->CurrentPhase;
    CachedActiveParticipantIndex = ResolvedSource->ActiveParticipantIndex;
    CachedTurnNumber = ResolvedSource->TurnNumber;
    CachedParticipants = ResolvedSource->Participants;
}

void UDWidget_TurnBasedParticipantManagerComponent::HandleTurnPhaseChanged(ETurnPhase NewPhase)
{
    RefreshAll();
}

void UDWidget_TurnBasedParticipantManagerComponent::HandleActiveControllerChanged(AController* NewActiveController)
{
    RefreshAll();
}

void UDWidget_TurnBasedParticipantManagerComponent::HandleParticipantForfeited(const FTurnParticipantInfo& ParticipantInfo)
{
    RefreshAll();
}

void UDWidget_TurnBasedParticipantManagerComponent::HandleAllParticipantsReady()
{
    RefreshAll();
}

void UDWidget_TurnBasedParticipantManagerComponent::HandleGameOver()
{
    RefreshAll();
}
