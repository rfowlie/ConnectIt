// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/DWidget_ConnectIt_GameState.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"

void UDWidget_ConnectIt_GameState::BindDelegates()
{
    ResolvedSource = UConnectIt_GameUtilityLibrary::GetConnectItGameState(this);

    if (!IsValid(ResolvedSource)) return;

    ResolvedSource->OnMatchPhaseChanged.AddDynamic(
        this, &UDWidget_ConnectIt_GameState::HandleMatchPhaseChanged);
    ResolvedSource->OnMatchResultUpdated.AddDynamic(
        this, &UDWidget_ConnectIt_GameState::HandleMatchResultUpdated);
}

void UDWidget_ConnectIt_GameState::UnbindDelegates()
{
    if (!IsValid(ResolvedSource)) return;

    ResolvedSource->OnMatchPhaseChanged.RemoveDynamic(
        this, &UDWidget_ConnectIt_GameState::HandleMatchPhaseChanged);
    ResolvedSource->OnMatchResultUpdated.RemoveDynamic(
        this, &UDWidget_ConnectIt_GameState::HandleMatchResultUpdated);
}

void UDWidget_ConnectIt_GameState::RefreshFields()
{
    bSourceValid = IsValid(ResolvedSource);
    if (!bSourceValid) return;

    CachedMatchPhase = ResolvedSource->GetMatchPhase();
    CachedMatchResult = ResolvedSource->MatchResult;
}

void UDWidget_ConnectIt_GameState::HandleMatchPhaseChanged(EMatchPhase NewPhase)
{
    RefreshAll();
}

void UDWidget_ConnectIt_GameState::HandleMatchResultUpdated(const FConnectItMatchResult& Result)
{
    RefreshAll();
}
