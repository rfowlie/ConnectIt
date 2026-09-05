// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/Rules/ConnectIt_BoardRulesComponent.h"

#include "Board/Rules/ConnectIt_LineScoringRule.h"
#include "Board/Rules/ConnectIt_ScoreThresholdWinCondition.h"


void UConnectIt_BoardRulesComponent::BeginPlay()
{
    Super::BeginPlay();

    // Set default rules
    if (!ScoringRule.GetObject())
    {
        ScoringRule = NewObject<UConnectIt_LineScoringRule>(
            this, UConnectIt_LineScoringRule::StaticClass());

        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_BoardRulesComponent: No scoring rule set "
                 "— defaulting to LineScoringRule"));
    }

    if (!WinConditionRule.GetObject())
    {
        WinConditionRule = NewObject<UConnectIt_ScoreThresholdWinCondition>(
            this, UConnectIt_ScoreThresholdWinCondition::StaticClass());

        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_BoardRulesComponent: No win condition set "
                 "— defaulting to ScoreThresholdWinCondition"));
    }
}

float UConnectIt_BoardRulesComponent::ApplyScoring(
    FConnectItBoardState& MutableState,
    FGridPosition Position,
    int32 FactionSlot,
    TArray<FGridPosition>& OutScoringPositions) const
{
    if (!ScoringRule.GetInterface())
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardRulesComponent: ApplyScoring — "
                 "no ScoringRule set"));
        return 0.f;
    }

    return IConnectIt_ScoringRule::Execute_ApplyScoring(
        ScoringRule.GetObject(), MutableState, Position, FactionSlot,
        OutScoringPositions);
}

void UConnectIt_BoardRulesComponent::CheckWinCondition(
    FConnectItBoardState& MutableState) const
{
    if (!WinConditionRule.GetInterface())
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardRulesComponent: CheckWinCondition — "
                 "no WinConditionRule set"));
        return;
    }

    IConnectIt_WinCondition::Execute_CheckWinCondition(
        WinConditionRule.GetObject(), MutableState);
}

float UConnectIt_BoardRulesComponent::GetTargetScore() const
{
    if (!WinConditionRule.GetInterface()) return 0.f;

    return IConnectIt_WinCondition::Execute_GetTargetScore(WinConditionRule.GetObject());
}

FName UConnectIt_BoardRulesComponent::GetActiveWinConditionName() const
{
    return WinConditionRule.GetObject()
        ? WinConditionRule.GetObject()->GetClass()->GetFName()
        : NAME_None;
}

FName UConnectIt_BoardRulesComponent::GetActiveScoringRuleName() const
{
    return ScoringRule.GetObject()
        ? ScoringRule.GetObject()->GetClass()->GetFName()
        : NAME_None;
}
