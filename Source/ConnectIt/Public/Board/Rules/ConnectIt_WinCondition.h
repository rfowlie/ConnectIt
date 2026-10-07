// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "ConnectIt_WinCondition.generated.h"


// How a match is won. A plain, thread-safe rule struct (see the note at the
// top of ConnectIt_ScoringRule.h) -- the server's Mediator and the AI's search
// ask the same instance.
//
// The base is generic on purpose: "who has won" and "how close is each
// faction", nothing about scores. Score-based conditions additionally answer
// GetTargetScore / SetTargetScore.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItWinCondition
{
    GENERATED_BODY()

    virtual ~FConnectItWinCondition() = default;

    // The faction slot that has won on Board, or INDEX_NONE if nobody has.
    virtual int32 GetWinningFaction(const FConnectItBoardState& Board) const
    {
        return INDEX_NONE;
    }

    // How close Faction is to winning, 0 (nowhere) to 1 (won) -- whatever
    // that means for this condition (score / target, tiles held / tiles
    // needed...). Feeds a generic progress bar.
    virtual float GetProgress(const FConnectItBoardState& Board, int32 Faction) const
    {
        return 0.f;
    }

    // Score-based conditions only: the score needed to win. False (and
    // OutTargetScore untouched) for a condition that isn't about score.
    virtual bool GetTargetScore(float& OutTargetScore) const
    {
        return false;
    }

    // Score-based conditions only: change the score needed to win (e.g. a
    // target chosen in the main menu's match setup). False if ignored.
    virtual bool SetTargetScore(float NewTargetScore)
    {
        return false;
    }
};
