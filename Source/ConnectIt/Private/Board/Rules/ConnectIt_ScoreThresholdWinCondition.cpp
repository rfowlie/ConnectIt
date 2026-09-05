// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/Rules/ConnectIt_ScoreThresholdWinCondition.h"


void UConnectIt_ScoreThresholdWinCondition::CheckWinCondition_Implementation(
    FConnectItBoardState& MutableState)
{
    // Refreshed on every check (i.e. every placement), not only on a win,
    // so UI sees an up-to-date target even if WinScoreThreshold is changed
    // at runtime (it's BlueprintReadWrite).
    MutableState.TargetScore = WinScoreThreshold;

    for (int32 i = 0; i < MutableState.ScoreBoard.Num(); i++)
    {
        if (MutableState.ScoreBoard[i] >= WinScoreThreshold)
        {
            MutableState.bGameOver         = true;
            MutableState.WinningFactionSlot = i;

            UE_LOG(LogTemp, Log,
                TEXT("ConnectIt_ScoreThresholdWinCondition: "
                     "Faction %d wins with %.0f points"),
                i, MutableState.ScoreBoard[i]);

            return;
        }
    }
}
