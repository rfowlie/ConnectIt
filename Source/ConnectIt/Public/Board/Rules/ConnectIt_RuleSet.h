// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "Board/Rules/ConnectIt_ScoringRule.h"
#include "Board/Rules/ConnectIt_TilePlaceableRule.h"
#include "Board/Rules/ConnectIt_WinCondition.h"
#include "ConnectIt_RuleSet.generated.h"


// The rules of one match: how pieces score, how the match is won, where a
// piece may be placed. Authored per level on UConnectIt_LevelConfigDataAsset
// (pick each rule's type and set its values in the Details panel).
//
// Plain data, copied by value: the level config holds the template, the
// GameMode takes its own copy per match (and may adjust it -- e.g. a target
// score chosen in the main menu -- without touching the asset), and the AI's
// search takes a copy of that. Everyone runs the same rule code, on any
// thread (see the note at the top of ConnectIt_ScoringRule.h).
//
// A fresh rule set is the classic game: Lines (4) / Score Threshold (100) /
// Unoccupied.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItRuleSet
{
    GENERATED_BODY()

    FConnectItRuleSet();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules", meta = (ExcludeBaseStruct))
    TInstancedStruct<FConnectItWinCondition> WinCondition;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules", meta = (ExcludeBaseStruct))
    TInstancedStruct<FConnectItScoringRule> ScoringRule;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules", meta = (ExcludeBaseStruct))
    TInstancedStruct<FConnectItTilePlaceableRule> TilePlaceableRule;

    // --- The rules themselves (null if a slot was cleared in the editor) ---

    const FConnectItWinCondition* GetWinCondition() const { return WinCondition.GetPtr(); }
    const FConnectItScoringRule* GetScoringRule() const { return ScoringRule.GetPtr(); }
    const FConnectItTilePlaceableRule* GetTilePlaceableRule() const { return TilePlaceableRule.GetPtr(); }

    // The scoring rule if it is a T (e.g. FConnectItScoringRule_Lines), else
    // null -- for code that only makes sense under one kind of scoring.
    template<typename T>
    const T* GetScoringRuleAs() const { return ScoringRule.template GetPtr<T>(); }

    // --- Convenience: each is a no-op / "no" when its rule is unset ---

    float ApplyScoring(
        FConnectItBoardState& Board,
        FGridPosition Position,
        int32 Faction,
        TArray<FGridPosition>& OutScoringPositions) const;

    bool IsTilePlaceable(const FConnectItBoardState& Board, FGridPosition Position) const;

    int32 GetWinningFaction(const FConnectItBoardState& Board) const;

    // 0 if the win condition isn't score-based
    float GetTargetScore() const;

    // False if the win condition isn't score-based
    bool SetTargetScore(float NewTargetScore);

    // The one place a win is written into board state, called by the Mediator
    // after a move's scoring: refreshes TargetScore and WinProgress, and sets
    // bGameOver / WinningFactionSlot once someone has won.
    void StampWinState(FConnectItBoardState& Board) const;
};
