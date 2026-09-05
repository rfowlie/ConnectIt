// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Board/Rules/ConnectIt_ScoringRule.h"
#include "Board/Rules/ConnectIt_WinCondition.h"
#include "ConnectIt_BoardRules.generated.h"

// Owns the board's pluggable scoring/win-condition strategies. Lives on
// AConnectIt_GameMode (server-only) alongside UConnectIt_BoardRequestMediator
// -- renamed and converted from UConnectIt_BoardRulesComponent, which used
// to sit on the now-retired AConnectIt_BoardManager actor. Never touched by
// clients -- its own accessors already documented this (see
// GetActiveWinConditionName/GetActiveScoringRuleName) before the move, so
// nothing about client-reachability changes here, only where this object
// lives.
UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API UConnectIt_BoardRules : public UObject
{
    GENERATED_BODY()

public:

    // Swappable scoring strategy -- defaults to UConnectIt_LineScoringRule
    // in Initialise() if left unset. Mirrors TurnBasedParticipantManagerComponent::
    // TurnOrderStrategy.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Rules",
        meta = (MustImplement = "/Script/ConnectIt.ConnectIt_ScoringRule"))
    TScriptInterface<IConnectIt_ScoringRule> ScoringRule;

    // Swappable win-condition strategy -- defaults to
    // UConnectIt_ScoreThresholdWinCondition in Initialise() if left unset.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Rules",
        meta = (MustImplement = "/Script/ConnectIt.ConnectIt_WinCondition"))
    TScriptInterface<IConnectIt_WinCondition> WinConditionRule;

    // Called once by whoever constructs this instance (AConnectIt_GameMode)
    // -- defaults ScoringRule/WinConditionRule if left unset. Replaces the
    // old component's BeginPlay, since a plain UObject has no such hook.
    void Initialise();

    // Wraps IConnectIt_ScoringRule::Execute_ApplyScoring -- Error-logs and
    // returns 0 if ScoringRule is unset (should not happen post-Initialise).
    // OutScoringPositions is appended with every tile from a completed line
    // (left untouched if nothing scored).
    float ApplyScoring(
        FConnectItBoardState& MutableState,
        FGridPosition Position,
        int32 FactionSlot,
        TArray<FGridPosition>& OutScoringPositions) const;

    // Wraps IConnectIt_WinCondition::Execute_CheckWinCondition -- Error-logs
    // and no-ops if WinConditionRule is unset (should not happen post-Initialise).
    void CheckWinCondition(FConnectItBoardState& MutableState) const;

    // Wraps IConnectIt_WinCondition::Execute_GetTargetScore -- 0 if unset.
    // Production UI should prefer FConnectItBoardState::TargetScore (this
    // object's own CheckWinCondition already stamps it there on every
    // check) -- that value is replicated; this call reaches a server-only
    // strategy object directly and is not.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Rules")
    float GetTargetScore() const;

    // Which strategies are configured -- debug visibility only, a
    // server-only object with no networked identity.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Rules|Debug")
    FName GetActiveWinConditionName() const;

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Rules|Debug")
    FName GetActiveScoringRuleName() const;
};
