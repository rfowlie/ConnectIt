// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "Board/Rules/ConnectIt_ScoringRule.h"
#include "Board/Rules/ConnectIt_MatchCondition.h"
#include "Board/Rules/ConnectIt_MatchConditionGroup.h"
#include "ConnectIt_RuleSet.generated.h"

struct FConnectItBoardOperation;
class UConnectIt_MatchConditionAsset;


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
// How a match is won: the Level director lists each side's goals (condition
// assets) in the order they are to be completed
// (UConnectIt_LevelDirector::PlayerWinConditions / AIWinConditions).
// Completing a goal that is not the last is a checkpoint; completing the last
// is the win. When the match starts, ResolveConditionGroups copies the assets'
// conditions into PlayerWinGroups / AIWinGroups, which is what everything
// plays with (server, clients, the AI's search); the level can replace a
// side's mid-match (SetConditionGroups). How far each faction has got lives
// on the board (FConnectItBoardState::GroupsCompleted), not here.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItRuleSet
{
    GENERATED_BODY()

    FConnectItRuleSet();

    // What the match plays with: the Level director's goals as plain groups,
    // copied by ResolveConditionGroups when the match starts (empty before). The
    // player's apply to every faction in a match with no AI (Online). Not
    // authored.
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Rules")
    TArray<FConnectItMatchConditionGroup> PlayerWinGroups;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Rules")
    TArray<FConnectItMatchConditionGroup> AIWinGroups;

    // The Level director's lose conditions as plain groups (any one met = that side
    // loses), copied by ResolveConditionGroups. Only apply in a match with an
    // AI (see GetLoseGroupsFor).
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Rules")
    TArray<FConnectItMatchConditionGroup> PlayerLoseGroups;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Rules")
    TArray<FConnectItMatchConditionGroup> AILoseGroups;

    // The faction slot the AI plays, set by the GameMode once it has
    // registered the AI (the slot isn't known until then). INDEX_NONE = no AI,
    // so PlayerWinGroups applies to every faction.
    UPROPERTY(BlueprintReadOnly, Category = "Rules")
    int32 AIFaction = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules", meta = (ExcludeBaseStruct))
    TInstancedStruct<FConnectItScoringRule> ScoringRule;

    // Copies the Level director's condition assets into the win and lose
    // groups, in order (empty entries skipped, with a warning). Game thread,
    // once per match, before the rules are used -- the GameMode does it.
    void ResolveConditionGroups(
        const TArray<TObjectPtr<UConnectIt_MatchConditionAsset>>& PlayerWinConditions,
        const TArray<TObjectPtr<UConnectIt_MatchConditionAsset>>& AIWinConditions,
        const TArray<TObjectPtr<UConnectIt_MatchConditionAsset>>& PlayerLoseConditions = {},
        const TArray<TObjectPtr<UConnectIt_MatchConditionAsset>>& AILoseConditions = {});

    // Replaces one side's win goals (bWinGoals) or lose conditions with
    // Conditions, resolved the same way. Game thread. The progress the board
    // keeps for them is the caller's to restart (the GameMode commits a
    // Change Goals operation).
    void SetConditionGroups(bool bForAI, bool bWinGoals,
        const TArray<TObjectPtr<UConnectIt_MatchConditionAsset>>& Conditions);

    // One side's win goals (bWinGoals) or lose conditions, in order
    const TArray<FConnectItMatchConditionGroup>& GetConditionGroups(bool bForAI, bool bWinGoals) const;

    // --- The rules themselves (null if a slot was cleared / left empty) ---

    const FConnectItScoringRule* GetScoringRule() const { return ScoringRule.GetPtr(); }

    // Replaces the scoring rule with NewRule (a scoring rule struct, e.g. Lines
    // with its settings). False, and nothing changed, if it isn't one.
    bool SetScoringRuleFrom(const FInstancedStruct& NewRule);
    static bool IsScoringRule(const FInstancedStruct& Rule);

    // The goals Faction has to complete: the AI's if Faction is the AI's slot,
    // else the player's. No fallback between them.
    const TArray<FConnectItMatchConditionGroup>& GetWinGroupsFor(int32 Faction) const;

    // The ways Faction loses: the AI's if Faction is the AI's slot, else the
    // player's -- and none at all in a match with no AI (lose conditions in
    // versus play are undecided, so they're ignored there).
    const TArray<FConnectItMatchConditionGroup>& GetLoseGroupsFor(int32 Faction) const;

    // The lowest faction that meets any of its lose conditions on Board, or
    // INDEX_NONE; OutGroup (if given) gets the condition met
    int32 GetLosingFaction(const FConnectItBoardState& Board, const FConnectItMatchConditionGroup** OutGroup = nullptr) const;

    // The scoring rule if it is a T (e.g. FConnectItScoringRule_Lines), else
    // null -- for code that only makes sense under one kind of scoring.
    template<typename T>
    const T* GetScoringRuleAs() const { return ScoringRule.template GetPtr<T>(); }

    // --- After a board change ---

    // The step AFTER an operation has changed the board
    // (FConnectItBoardOperation::Apply), kept separate from the change
    // itself, starting from what the operation reported (Changes). A loop:
    //   1. settle -- pre-scoring reactions to what changed (tile powers:
    //      empty until they are built);
    //   2. score -- one scoring pass over every piece that arrived since the
    //      last pass (FConnectItScoringRule::ScorePass: all found first, then
    //      all applied);
    //   3. scoring reactions (tile powers: empty until built);
    //   then back to 1 with what scoring and its reactions changed. The chain
    //   ends at the first pass that scores nothing (a bug guard stops it
    //   after 64 passes).
    // Returns the points scored; OutEvents, if given, gets one Scored event
    // appended per thing that scored (the AI's search passes none). Does not
    // check for a win -- see GetWinningFaction / StampWinState.
    float ResolveBoardChange(
        FConnectItBoardState& Board,
        const FConnectItBoardChanges& Changes,
        FConnectItBoardChangeEvent* OutEvents = nullptr) const;

    // An operation carried out on Board under these rules: the operation
    // changes the board (FConnectItBoardOperation::Apply, its faction stamped
    // on the change record), then ResolveBoardChange resolves what it did. Returns the points scored;
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

    // One scoring pass for the piece on Position alone (it stays if it scores
    // and bArrivingPieceSurvives)
    float ApplyScoring(
        FConnectItBoardState& Board,
        FGridPosition Position,
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

    // The faction slot that has won, or INDEX_NONE: a faction that has
    // completed all its goals (if several, the lowest slot) -- or, if none
    // has, the other side of a faction that has lost (two factions). Wins are
    // checked first, so a move that completes a win and meets a lose
    // condition is a win. The AI's search asks this too, so it sees losses.
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
    // sets bGameOver / WinningFactionSlot once someone has won. A win by a
    // loss also sets LosingFactionSlot and appends a LoseConditionMet event.
    void StampWinState(FConnectItBoardState& Board, FConnectItBoardChangeEvent* OutEvents = nullptr) const;
};
