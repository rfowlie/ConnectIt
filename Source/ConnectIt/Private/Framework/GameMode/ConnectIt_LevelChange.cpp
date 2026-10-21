// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/GameMode/ConnectIt_LevelChange.h"


bool FConnectItLevelChangeQueue::Add(const FConnectItLevelChange& Change, EConnectItChangeTiming Timing, bool bBusy)
{
    if (Timing == EConnectItChangeTiming::NextTurnStart)
    {
        NextTurn.Add(Change);
        return false;
    }

    // Behind anything already waiting, so changes keep the order they were asked in
    if (bBusy || !Waiting.IsEmpty())
    {
        Waiting.Add(Change);
        return false;
    }
    return true;
}

void FConnectItLevelChangeQueue::StartTurn()
{
    Waiting.Append(MoveTemp(NextTurn));
    NextTurn.Reset();
}

bool FConnectItLevelChangeQueue::TakeNext(FConnectItLevelChange& OutChange)
{
    if (Waiting.IsEmpty()) return false;

    OutChange = Waiting[0];
    Waiting.RemoveAt(0);
    return true;
}

int32 FConnectItLevelChangeQueue::DiscardWaiting()
{
    const int32 Discarded = Waiting.Num();
    Waiting.Reset();
    return Discarded;
}
