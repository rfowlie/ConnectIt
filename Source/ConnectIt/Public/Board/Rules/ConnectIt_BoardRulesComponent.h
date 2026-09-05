// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Board/Rules/ConnectIt_ScoringRule.h"
#include "Board/Rules/ConnectIt_WinCondition.h"
#include "ConnectIt_BoardRulesComponent.generated.h"

// Owns the board's pluggable scoring/win-condition strategies, one sibling
// component alongside AConnectIt_BoardManager's BoardStateComponent/
// BoardShiftComponent/ConnectItConfig. AConnectIt_BoardManager calls only
// the two plain wrapper methods below -- it never touches
// IConnectIt_ScoringRule/IConnectIt_WinCondition or Execute_ directly, same
// separation it already has with its other components.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CONNECTIT_API UConnectIt_BoardRulesComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    // Swappable scoring strategy -- defaults to UConnectIt_LineScoringRule
    // in BeginPlay if left unset. Mirrors TurnBasedParticipantManagerComponent::
    // TurnOrderStrategy.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Rules",
        meta = (MustImplement = "/Script/ConnectIt.ConnectIt_ScoringRule"))
    TScriptInterface<IConnectIt_ScoringRule> ScoringRule;

    // Swappable win-condition strategy -- defaults to
    // UConnectIt_ScoreThresholdWinCondition in BeginPlay if left unset.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Rules",
        meta = (MustImplement = "/Script/ConnectIt.ConnectIt_WinCondition"))
    TScriptInterface<IConnectIt_WinCondition> WinConditionRule;

    // Wraps IConnectIt_ScoringRule::Execute_ApplyScoring -- Error-logs and
    // returns 0 if ScoringRule is unset (should not happen post-BeginPlay).
    // OutScoringPositions is appended with every tile from a completed line
    // (left untouched if nothing scored).
    float ApplyScoring(
        FConnectItBoardState& MutableState,
        FGridPosition Position,
        int32 FactionSlot,
        TArray<FGridPosition>& OutScoringPositions) const;

    // Wraps IConnectIt_WinCondition::Execute_CheckWinCondition -- Error-logs
    // and no-ops if WinConditionRule is unset (should not happen post-BeginPlay).
    void CheckWinCondition(FConnectItBoardState& MutableState) const;

    // Wraps IConnectIt_WinCondition::Execute_GetTargetScore -- 0 if unset.
    // Production UI should prefer FConnectItBoardState::TargetScore (this
    // component's own CheckWinCondition already stamps it there on every
    // check) -- that value is replicated; this call reaches a server-side
    // strategy object directly and is not.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Rules")
    float GetTargetScore() const;

    // Which strategies are configured. Config/debug visibility ONLY -- these
    // read a server-side object whose identity is not networked, so a
    // client may see a defaulted strategy (BeginPlay's fallback) rather than
    // whatever was actually authored server-side. Production UI must read
    // FConnectItBoardState::TargetScore instead, which IS replicated.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Rules|Debug")
    FName GetActiveWinConditionName() const;

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Rules|Debug")
    FName GetActiveScoringRuleName() const;

protected:

    virtual void BeginPlay() override;
};
