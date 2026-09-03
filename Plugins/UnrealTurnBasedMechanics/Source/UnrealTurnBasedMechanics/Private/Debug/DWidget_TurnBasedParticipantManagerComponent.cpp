// Fill out your copyright notice in the Description page of Project Settings.

#include "Debug/DWidget_TurnBasedParticipantManagerComponent.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"
#include "GameFramework/GameStateBase.h"


void UDWidget_TurnBasedParticipantManagerComponent::BindDelegates()
{
    AGameStateBase* GameState = GetWorld()->GetGameState<AGameStateBase>();
    if (!IsValid(GameState))
    {
        UE_LOG(LogDWidget, Error,
            TEXT("UDWidget_TurnBasedParticipantManagerComponent::BindDelegates - GameState Invalid"));
        return;
    }

    // GetWorld()->OnWorldBeginPlay.AddUFunction(this, "BindDelegates");

    ResolvedSource = GameState->GetComponentByClass<UTurnBasedParticipantManagerComponent>();
    bSourceValid = IsValid(ResolvedSource);
    if (!bSourceValid)
    {
        UE_LOG(LogDWidget, Error,
            TEXT("UDWidget_TurnBasedParticipantManagerComponent::BindDelegates - Source Invalid"));
        return;
    }

    ResolvedSource->OnTurnPhaseChanged.AddDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleTurnPhaseChanged);
    ResolvedSource->OnActiveControllerChanged.AddDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleActiveControllerChanged);
    ResolvedSource->OnParticipantForfeited.AddDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleParticipantForfeited);
    ResolvedSource->OnParticipantIndexChanged.AddDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::OnActiveParticipantIndexUpdated);
    ResolvedSource->OnAllParticipantsReady.AddDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleAllParticipantsReady);
    ResolvedSource->OnGameOver.AddDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleGameOver);

    PushCurrentInfo();
}

void UDWidget_TurnBasedParticipantManagerComponent::UnbindDelegates()
{
    if (!IsValid(ResolvedSource))
    {
        UE_LOG(LogDWidget, Error,
            TEXT("UDWidget_TurnBasedParticipantManagerComponent::UnbindDelegates - Source Invalid"));
        return;
    }

    ResolvedSource->OnTurnPhaseChanged.RemoveDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleTurnPhaseChanged);
    ResolvedSource->OnActiveControllerChanged.RemoveDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleActiveControllerChanged);
    ResolvedSource->OnParticipantForfeited.RemoveDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleParticipantForfeited);
    ResolvedSource->OnParticipantIndexChanged.RemoveDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::OnActiveParticipantIndexUpdated);
    ResolvedSource->OnAllParticipantsReady.RemoveDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleAllParticipantsReady);
    ResolvedSource->OnGameOver.RemoveDynamic(
        this, &UDWidget_TurnBasedParticipantManagerComponent::HandleGameOver);
}

void UDWidget_TurnBasedParticipantManagerComponent::HandleTurnPhaseChanged(ETurnPhase NewPhase)
{
    OnTurnPhaseUpdated(NewPhase);
}

void UDWidget_TurnBasedParticipantManagerComponent::HandleActiveControllerChanged(AController* NewActiveController)
{
    if (!IsValid(ResolvedSource)) return;

    OnActiveParticipantIndexUpdated(ResolvedSource->ActiveParticipantIndex);
    OnTurnNumberUpdated(ResolvedSource->TurnNumber);
}

void UDWidget_TurnBasedParticipantManagerComponent::HandleParticipantForfeited(const FTurnParticipantInfo& ParticipantInfo)
{
    if (!IsValid(ResolvedSource)) return;

    OnParticipantsUpdated(ResolvedSource->Participants);
}

void UDWidget_TurnBasedParticipantManagerComponent::HandleAllParticipantsReady()
{
    PushCurrentInfo();
}

void UDWidget_TurnBasedParticipantManagerComponent::HandleGameOver()
{
    PushCurrentInfo();
}

void UDWidget_TurnBasedParticipantManagerComponent::PushCurrentInfo()
{
    if (!IsValid(ResolvedSource))
    {
        UE_LOG(LogDWidget, Error,
            TEXT("UDWidget_TurnBasedParticipantManagerComponent::PushCurrentInfo - Source Invalid"));
        return;
    }

    const FTurnBasedParticipantManagerInfo Info = ResolvedSource->GetInfo();
    OnTurnPhaseUpdated(Info.TurnPhase);
    OnActiveParticipantIndexUpdated(Info.ActiveParticipantIndex);
    OnTurnNumberUpdated(Info.TurnNumber);
    OnParticipantsUpdated(Info.Participants);
}
