// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "Board/Rules/ConnectIt_ScoringRule.h"
#include "Board/Rules/ConnectIt_WinCondition.h"
#include "Board/Rules/ConnectIt_WinConditionGroup.h"
#include "ConnectIt_RuleSet.generated.h"

struct FConnectItBoardOperation;


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
// A fresh rule set scores with Lines (4) and has NO win groups: each level
// config sets its own (a side with none can't win, and nothing fills in for
// it). (Which moves a player can make is not a rule: it is their loadout's
// actions, each sending a FConnectItBoardOperation.)
//
// How a match is won: each side lists its win condition GROUPS in the order they
// are to be completed. Completing a group that is not the last is a checkpoint;
// completing the last is the win. A side that wants a single goal lists a single
// group. How far each faction has got lives on the board
// (FConnectItBoardState::GroupsCompleted), not here.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItRuleSet
{
    GENERATED_BODY()

    FConnectItRuleSet();

    // The player's goals, in order. Also the goals of every faction in a match
    // with no AI (Online).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules")
    TArray<FConnectItWinConditionGroup> PlayerWinGroups;

    // The AI's goals, in order. Set them whenever the match has an AI: an empty
    // list means the AI cannot win (it does not borrow the player's).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules")
    TArray<FConnectItWinConditionGroup> AIWinGroups;

    // The faction slot the AI plays, set by the GameMode once it has
    // registered the AI (the slot isn't known until then). INDEX_NONE = no AI,
    // so PlayerWinGroups applies to every faction.
    UPROPERTY(BlueprintReadOnly, Category = "Rules")
    int32 AIFaction = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules", meta = (ExcludeBaseStruct))
    TInstancedStruct<FConnectItScoringRule> ScoringRule;

    // --- The rules themselves (null if a slot was cleared / left empty) ---

    const FConnectItScoringRule* GetScoringRule() const { return ScoringRule.GetPtr(); }

    // The goals Faction has to complete: the AI's if Faction is the AI's slot,
    // else the player's. No fallback between them.
    const TArray<FConnectItWinConditionGroup>& GetWinGroupsFor(int32 Faction) const;

    // The scoring rule if it is a T (e.g. FConnectItScoringRule_Lines), else
    // null -- for code that only makes sense under one kind of scoring.
    template<typename T>
    const T* GetScoringRuleAs() const { return ScoringRule.template GetPtr<T>(); }

    // --- After a board change ---

    // The step AFTER an operation has changed the board
    // (FConnectItBoardOperation::Apply), kept separate from the change
    // itself: every touched position that now holds a faction's piece is
    // scored for that faction, in order. bArrivingPiecesSurvive is the
    // operation's own answer (FConnectItBoardOperation::
    // ArrivingPiecesSurviveScoring): whether a touched piece that scores
    // stays on its tile. Other
    // follow-on effects of a board change (reactions) will run here too.
    // Returns the points scored; OutEvents, if given, gets one Scored event
    // appended per thing that scored (the AI's search passes none). Does not check for
    // a win -- see GetWinningFaction / StampWinState.
    float ResolveBoardChange(
        FConnectItBoardState& Board,
        TConstArrayView<FGridPosition> TouchedPositions,
        bool bArrivingPiecesSurvive,
        FConnectItBoardChangeEvent* OutEvents = nullptr) const;

    // An operation carried out on Board under these rules: the operation
    // changes the board (FConnectItBoardOperation::Apply), then
    // ResolveBoardChange scores what it touched. Returns the points scored;
    // OutEvents, if given, gets the operation's event followed by any Scored
    // events. Only call when Operation.CanApply(Board). Does not check for a
    // win (StampWinState). The server's Mediator and the Blueprint
    // "Simulate Operation" both go through here, so a simulated move is by
    // construction what the real one would do.
    float CarryOutOperation(
        const FConnectItBoardOperation& Operation,
        FConnectItBoardState& Board,
        FConnectItBoardChangeEvent* OutEvents = nullptr) const;

    // --- Convenience: each is a no-op / "no" when its rule is unset ---

    float ApplyScoring(
        FConnectItBoardState& Board,
        FGridPosition Position,
        int32 Faction,
        bool bArrivingPieceSurvives,
        FConnectItBoardChangeEvent* OutEvents = nullptr) const;

    // Moves each faction on through its win groups: while the next group is
    // satisfied on Board, it is completed (Board.GroupsCompleted, which only
    // ever goes up). One call can complete several groups in a row. A group
    // completed that is not the faction's last appends a WinCheckpointReached
    // event to OutEvents, if given; completing the last appends nothing (that
    // is the win, see StampWinState). The AI's search calls this too, without
    // events.
    void AdvanceWinGroups(FConnectItBoardState& Board, FConnectItBoardChangeEvent* OutEvents = nullptr) const;

    // Whether Faction has completed all of ITS win groups (a plain read of
    // Board.GroupsCompleted: AdvanceWinGroups does the completing). False for a
    // faction with no groups.
    bool HasFactionWon(const FConnectItBoardState& Board, int32 Faction) const;

    // The faction slot that has won, or INDEX_NONE. If several have, the
    // lowest slot wins.
    int32 GetWinningFaction(const FConnectItBoardState& Board) const;

    // How close Faction is to winning, 0 to 1: groups completed, plus how far
    // through the current one, over the number of groups.
    float GetProgress(const FConnectItBoardState& Board, int32 Faction) const;

    // The score target UI should show on Board: the current group's target for
    // the first faction whose current group has a score-based condition (the
    // last group's once a faction has completed them all); 0 if none does.
    // (One number for UI that shows a single target; per-faction UI should read
    // the board's WinProgress.)
    float GetTargetScore(const FConnectItBoardState& Board) const;

    // The same before any group is complete: the first group's target, the
    // player's list first. Used to seed the board.
    float GetTargetScore() const;

    // Sets the target score of every score-based condition in the LAST group of
    // each list (the final goal: what a main-menu target means). Checkpoint
    // groups keep their designed values. False if there is none.
    bool SetTargetScore(float NewTargetScore);

    // The one place a win is written into board state, called by the Mediator
    // after a move's scoring: advances the win groups (appending checkpoint
    // events to OutEvents, if given), refreshes TargetScore and WinProgress, and
    // sets bGameOver / WinningFactionSlot once someone has won.
    void StampWinState(FConnectItBoardState& Board, FConnectItBoardChangeEvent* OutEvents = nullptr) const;
};
