// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/Rules/ConnectIt_ScoreThresholdWinCondition.h"


void UConnectIt_ScoreThresholdWinCondition::CheckWinCondition_Implementation(
    FConnectItBoardState& MutableState)
{
    // Refreshed on every check (i.e. every placement), not only on a win,
    // so UI sees an up-to-date target even if WinScoreThreshold is changed
    // at runtime (it's BlueprintReadWrite).
    MutableState.TargetScore = WinScoreThreshold;

    const int32 Winner = GetWinningFaction(MutableState.ScoreBoard, WinScoreThreshold);
    if (Winner != INDEX_NONE)
    {
        MutableState.bGameOver = true;
        MutableState.WinningFactionSlot = Winner;

        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_ScoreThresholdWinCondition: "
                 "Faction %d wins with %.0f points"),
            Winner, MutableState.ScoreBoard[Winner]);
    }
}

TSharedPtr<const FConnectItWinCheck, ESPMode::ThreadSafe>
UConnectIt_ScoreThresholdWinCondition::MakeSearchWinCheck() const
{
    return MakeShared<const FConnectItScoreThresholdWinCheck, ESPMode::ThreadSafe>(WinScoreThreshold);
}

int32 UConnectIt_ScoreThresholdWinCondition::GetWinningFaction(
    const TArray<float>& ScoreBoard, float Threshold)
{
    for (int32 i = 0; i < ScoreBoard.Num(); i++)
    {
        if (ScoreBoard[i] >= Threshold)
        {
            return i;
        }
    }
    return INDEX_NONE;
}
