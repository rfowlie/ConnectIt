// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/Rules/ConnectIt_BoardRules.h"

#include "Board/Rules/ConnectIt_LineScoringRule.h"
#include "Board/Rules/ConnectIt_ScoreThresholdWinCondition.h"
#include "Board/Rules/ConnectIt_UnoccupiedTilePlaceableRule.h"


void UConnectIt_BoardRules::Initialise()
{
    // Set default rules
    if (!ScoringRule)
    {
        ScoringRule = NewObject<UConnectIt_LineScoringRule>(
            this, UConnectIt_LineScoringRule::StaticClass());

        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_BoardRules: No scoring rule set "
                 "— defaulting to LineScoringRule"));
    }

    if (!WinConditionRule)
    {
        WinConditionRule = NewObject<UConnectIt_ScoreThresholdWinCondition>(
            this, UConnectIt_ScoreThresholdWinCondition::StaticClass());

        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_BoardRules: No win condition set "
                 "— defaulting to ScoreThresholdWinCondition"));
    }

    if (!TilePlaceableRule)
    {
        TilePlaceableRule = NewObject<UConnectIt_UnoccupiedTilePlaceableRule>(
            this, UConnectIt_UnoccupiedTilePlaceableRule::StaticClass());

        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_BoardRules: No tile-placeable rule set "
                 "— defaulting to UnoccupiedTilePlaceableRule"));
    }
}

float UConnectIt_BoardRules::ApplyScoring(
    FConnectItBoardState& MutableState,
    FGridPosition Position,
    int32 FactionSlot,
    TArray<FGridPosition>& OutScoringPositions) const
{
    if (!ScoringRule)
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardRules: ApplyScoring — "
                 "no ScoringRule set"));
        return 0.f;
    }

    return IConnectIt_ScoringRule::Execute_ApplyScoring(
        ScoringRule, MutableState, Position, FactionSlot,
        OutScoringPositions);
}

void UConnectIt_BoardRules::CheckWinCondition(
    FConnectItBoardState& MutableState) const
{
    if (!WinConditionRule)
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardRules: CheckWinCondition — "
                 "no WinConditionRule set"));
        return;
    }

    IConnectIt_WinCondition::Execute_CheckWinCondition(
        WinConditionRule, MutableState);
}

float UConnectIt_BoardRules::GetTargetScore() const
{
    if (!WinConditionRule) return 0.f;

    return IConnectIt_WinCondition::Execute_GetTargetScore(WinConditionRule);
}

bool UConnectIt_BoardRules::IsTilePlaceable(
    const FConnectItBoardState& State, FGridPosition Position) const
{
    if (!TilePlaceableRule)
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardRules: IsTilePlaceable — "
                 "no TilePlaceableRule set"));
        return false;
    }

    return IConnectIt_TilePlaceableRule::Execute_IsTilePlaceable(
        TilePlaceableRule, State, Position);
}

FName UConnectIt_BoardRules::GetActiveWinConditionName() const
{
    return WinConditionRule
        ? WinConditionRule->GetClass()->GetFName()
        : NAME_None;
}

FName UConnectIt_BoardRules::GetActiveScoringRuleName() const
{
    return ScoringRule
        ? ScoringRule->GetClass()->GetFName()
        : NAME_None;
}
