// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "Board/Rules/ConnectIt_ScoringRule.h"
#include "Board/Rules/ConnectIt_WinCondition.h"
#include "ConnectIt_RuleSet.generated.h"


// The rules of one match: how pieces score and how the match is won.
// Authored per level on UConnectIt_LevelConfigDataAsset
// (pick each rule's type and set its values in the Details panel).
//
// Plain data, copied by value: the level config holds the template, the
// GameMode takes its own copy per match (and may adjust it -- e.g. a target
// score chosen in the main menu -- without touching the asset), and the AI's
// search takes a copy of that. Everyone runs the same rule code, on any
// thread (see the note at the top of ConnectIt_ScoringRule.h).
//
// A fresh rule set is the classic game: Lines (4) / Score Threshold (100).
// (Which moves a player can make is not a rule: it is their loadout's
// actions, each sending a FConnectItBoardOperation.)
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItRuleSet
{
    GENERATED_BODY()

    FConnectItRuleSet();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules", meta = (ExcludeBaseStruct))
    TInstancedStruct<FConnectItWinCondition> WinCondition;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules", meta = (ExcludeBaseStruct))
    TInstancedStruct<FConnectItScoringRule> ScoringRule;

    // --- The rules themselves (null if a slot was cleared in the editor) ---

    const FConnectItWinCondition* GetWinCondition() const { return WinCondition.GetPtr(); }
    const FConnectItScoringRule* GetScoringRule() const { return ScoringRule.GetPtr(); }

    // The scoring rule if it is a T (e.g. FConnectItScoringRule_Lines), else
    // null -- for code that only makes sense under one kind of scoring.
    template<typename T>
    const T* GetScoringRuleAs() const { return ScoringRule.template GetPtr<T>(); }

    // --- After a board change ---

    // The step AFTER an operation has changed the board
    // (FConnectItBoardOperation::Apply), kept separate from the change
    // itself: every touched position that now holds a faction's piece is
    // scored for that faction, in order. Other
    // follow-on effects of a board change (reactions) will run here too.
    // Returns the points scored; OutConfigurations, if given, gets one entry
    // per thing that scored (the AI's search passes none). Does not check for
    // a win -- see GetWinningFaction / StampWinState.
    float ResolveBoardChange(
        FConnectItBoardState& Board,
        TConstArrayView<FGridPosition> TouchedPositions,
        TArray<FConnectItScoringConfiguration>* OutConfigurations = nullptr) const;

    // --- Convenience: each is a no-op / "no" when its rule is unset ---

    float ApplyScoring(
        FConnectItBoardState& Board,
        FGridPosition Position,
        int32 Faction,
        TArray<FConnectItScoringConfiguration>* OutConfigurations = nullptr) const;

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
