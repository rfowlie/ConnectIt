// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/DWidget_ConnectIt_GameState.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"


void UDWidget_ConnectIt_GameState::BindDelegates()
{
    ResolvedSource = UConnectIt_GameUtilityLibrary::GetConnectItGameState(this);
    bSourceValid = IsValid(ResolvedSource);

    if (!bSourceValid)
    {
        UE_LOG(LogDWidget, Error,
            TEXT("UDWidget_ConnectIt_GameState::BindDelegates - Source Invalid"));
        return;
    }

    ResolvedSource->OnMatchPhaseChanged.AddDynamic(
        this, &UDWidget_ConnectIt_GameState::OnMatchPhaseUpdated);
    ResolvedSource->OnMatchResultUpdated.AddDynamic(
        this, &UDWidget_ConnectIt_GameState::OnMatchResultUpdated);

    // Seed initial values through the same events future changes use.
    const FConnectItGameStateInfo Info = ResolvedSource->GetInfo();
    OnMatchPhaseUpdated(Info.MatchPhase);
    OnMatchResultUpdated(Info.MatchResult);
}

void UDWidget_ConnectIt_GameState::UnbindDelegates()
{
    if (!IsValid(ResolvedSource)) return;

    ResolvedSource->OnMatchPhaseChanged.RemoveDynamic(
        this, &UDWidget_ConnectIt_GameState::OnMatchPhaseUpdated);
    ResolvedSource->OnMatchResultUpdated.RemoveDynamic(
        this, &UDWidget_ConnectIt_GameState::OnMatchResultUpdated);
}
