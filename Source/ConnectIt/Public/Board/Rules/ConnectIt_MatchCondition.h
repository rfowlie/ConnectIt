// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "ConnectIt_MatchCondition.generated.h"

struct FConnectItRuleSet;

// What a condition is asked about: the board, and the match's rules (e.g. how
// many pieces a score needs). References valid for the call only. Plain data,
// safe on any thread like the rules themselves -- the AI's search asks too.
struct FConnectItConditionContext
{
    const FConnectItBoardState& Board;
    const FConnectItRuleSet& Rules;
};

// Something that holds, or doesn't, for a faction on a board -- the building
// block of a match's goals (and, later, of its losses and level triggers).
// Formerly FConnectItWinCondition. A plain, thread-safe rule struct (see the
// note at the top of ConnectIt_ScoringRule.h) -- the server's Mediator and the
// AI's search ask the same instance.
//
// The base is generic on purpose: "does it hold" and "how close is each
// faction", nothing about scores. Score-based conditions additionally answer
// GetTargetScore / SetTargetScore.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItMatchCondition
{
    GENERATED_BODY()

    virtual ~FConnectItMatchCondition() = default;

    // Whether the condition holds for Faction, independent of any other
    // faction (the player and the AI can have different conditions). Default:
    // never.
    virtual bool IsMetFor(const FConnectItConditionContext& Context, int32 Faction) const
    {
        return false;
    }

    // How close Faction is to meeting it, 0 (nowhere) to 1 (met) -- whatever
    // that means for this condition (score / target, pieces cleared...).
    // Feeds a generic progress bar.
    virtual float GetProgress(const FConnectItConditionContext& Context, int32 Faction) const
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

    // Problems a designer should know about, shown when the asset holding
    // this condition is validated. Rules: the match's rules when a match
    // preset validates it, null when the asset is validated on its own.
    virtual void GetValidationWarnings(const FConnectItRuleSet* Rules, TArray<FText>& OutWarnings) const
    {
    }
};
