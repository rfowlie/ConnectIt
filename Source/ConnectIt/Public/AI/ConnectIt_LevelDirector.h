// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/ConnectIt_Director.h"
#include "StructUtils/InstancedStruct.h"
#include "Framework/GameMode/ConnectIt_GameMode.h"
#include "Framework/GameMode/ConnectIt_LevelChange.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "ConnectIt_LevelDirector.generated.h"

class UConnectIt_MatchConditionAsset;
class UConnectIt_BoardLayout;
struct FConnectItBoardState;
struct FConnectItRuleSet;

// Whose pieces / score a level trigger's condition is checked for
UENUM(BlueprintType)
enum class EConnectItTriggerSide : uint8
{
    // The human side (every faction in a match with no AI)
    Player  UMETA(DisplayName = "Player"),
    // The AI's faction
    AI      UMETA(DisplayName = "AI"),
    // Any faction
    Either  UMETA(DisplayName = "Either"),
};

// One level trigger: a condition the level watches for, and whose side
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItLevelTrigger
{
    GENERATED_BODY()

    // Met when all of its conditions hold (e.g. Score Threshold 10: "the
    // player has reached 10 points")
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger")
    TObjectPtr<UConnectIt_MatchConditionAsset> Condition = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger")
    EConnectItTriggerSide Side = EConnectItTriggerSide::Player;
};

// The level's own script: reacts to the match (the shared director events,
// anything it can reach through the world) and, being the level, is the one
// director that changes the board and the rules -- not a side's loadout
// (ask that side's director). One instance per match, owned by the GameMode;
// named by class on the match preset. Subclass it in Blueprint per adventure
// level, plus a generic one per standard way of playing (classic, versus).
//
// Goals: it holds both sides' win goals and lose conditions -- a level's whole
// script in one place, so the Player / AI directors are reusable loadout and
// AI setup. Set Win Conditions / Set Lose Conditions replace a side's
// mid-match. With no Level Director Class on the preset, nobody has goals.
//
// Triggers: after every committed board change, each armed trigger whose
// condition now holds (for its side) fires On Trigger Fired once and disarms;
// Rearm Trigger arms it again. Fired / armed state lives here. Triggers are
// the level's own business: they don't win or lose anything, and the AI's
// search doesn't see them.
//
// Changes: Apply Board Operation (e.g. Force Place Piece, Toggle Tile
// Active, Destroy Tile Multiplier -- Make Instanced Struct) and Set Scoring
// Rule, each at a timing: Immediately (right after the move being carried
// out, if any) or at the next turn's start. Board operations go through the
// same checks, scoring, win check and events as a move, without a loadout.
UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API UConnectIt_LevelDirector : public UConnectIt_Director
{
    GENERATED_BODY()

public:

    // --- Goals (each side's, at the start of the match) ---

    // The player's goals, in the order they are completed: completing one
    // that isn't the last is a checkpoint, completing the last is the win.
    // Also every faction's goals in a match with no AI.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Conditions")
    TArray<TObjectPtr<UConnectIt_MatchConditionAsset>> PlayerWinConditions;

    // Ways the player LOSES: if any one holds after a move (all of its
    // conditions at once), the player loses and the other side wins -- unless
    // a win was completed in that same move (wins are checked first). Only in
    // a match with an AI (ignored online for now). E.g. Not Enough Pieces To
    // Score, No Legal Move: Swap Pieces on an exhaustion level.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Conditions")
    TArray<TObjectPtr<UConnectIt_MatchConditionAsset>> PlayerLoseConditions;

    // The AI's goals, in order. Set them: the AI never borrows the player's,
    // and none means it can't win.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Conditions")
    TArray<TObjectPtr<UConnectIt_MatchConditionAsset>> AIWinConditions;

    // Ways the AI loses (as Player Lose Conditions)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Conditions")
    TArray<TObjectPtr<UConnectIt_MatchConditionAsset>> AILoseConditions;

    // --- Tile powers ---

    // Powers that apply to every tile, on top of a tile's own (e.g. Crumble),
    // in firing order: their tags -- the level's tile catalog defines them.
    // Copied into the match's rules when it starts.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Tile Powers", meta = (Categories = "ConnectIt.TilePower"))
    TArray<FGameplayTag> LevelWideTilePowers;

    // --- Triggers ---

    // What the level watches for. Each fires once (until re-armed).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Triggers")
    TArray<FConnectItLevelTrigger> Triggers;

    // A trigger's condition held: Condition is the trigger's asset, Faction the
    // side it held for. The trigger is now disarmed.
    UFUNCTION(BlueprintNativeEvent, Category = "Level|Triggers")
    void OnTriggerFired(UConnectIt_MatchConditionAsset* Condition, int32 Faction);
    virtual void OnTriggerFired_Implementation(UConnectIt_MatchConditionAsset* Condition, int32 Faction) {}

    // Arms again every trigger using Condition, so it can fire again the next
    // time its condition holds after a board change
    UFUNCTION(BlueprintCallable, Category = "Level|Triggers")
    void RearmTrigger(UConnectIt_MatchConditionAsset* Condition);

    // Whether the trigger using Condition is armed (false if none uses it)
    UFUNCTION(BlueprintPure, Category = "Level|Triggers")
    bool IsTriggerArmed(UConnectIt_MatchConditionAsset* Condition) const;

    // Checks every armed trigger against a committed board and the match's
    // rules, firing the ones that hold. Called by the GameMode after each
    // board change (nothing after the game is over).
    void CheckTriggers(const FConnectItBoardState& Board, const FConnectItRuleSet& Rules);

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
    virtual void AddConditionRuleWarnings(const FConnectItRuleSet& Rules, FDataValidationContext& Context) const override;
#endif

protected:

    // Changes the board on the level's behalf: Operation is a board operation
    // struct (Make Instanced Struct), made by AsFaction (-1 for one that
    // belongs to nobody, e.g. Toggle Tile Active). It is checked, scores and
    // can win like a move, but needs no loadout. Immediately = right after the
    // move being carried out, if any.
    UFUNCTION(BlueprintCallable, Category = "Level|Changes")
    void ApplyBoardOperation(
        UPARAM(meta = (BaseStruct = "/Script/ConnectIt.ConnectItBoardOperation")) const FInstancedStruct& Operation,
        int32 AsFaction = -1,
        EConnectItChangeTiming Timing = EConnectItChangeTiming::Immediately);

    // Replaces the match's scoring rule (a scoring rule struct, e.g. Lines
    // with its settings). Clients see it through the replicated rules (On
    // Match Rules Changed); the AI from its next decision. False if it isn't
    // a scoring rule.
    UFUNCTION(BlueprintCallable, Category = "Level|Changes")
    bool SetScoringRule(
        UPARAM(meta = (BaseStruct = "/Script/ConnectIt.ConnectItScoringRule")) const FInstancedStruct& ScoringRule,
        EConnectItChangeTiming Timing = EConnectItChangeTiming::NextTurnStart);

    // Replaces Side's goals, in order (completing the last is the win). That
    // side's goal progress restarts at 0 -- score carries over, so a score
    // goal already reached completes at once. A board change like any other:
    // it plays a Goals Changed event, then any checkpoint or win the new goals
    // already meet. Set them before changing the layout in the same moment, so
    // the new layout is checked against the new goals.
    UFUNCTION(BlueprintCallable, Category = "Level|Changes")
    void SetWinConditions(EConnectItRole Side, const TArray<UConnectIt_MatchConditionAsset*>& Conditions,
        EConnectItChangeTiming Timing = EConnectItChangeTiming::Immediately);

    // Replaces Side's lose conditions (any one met = that side loses; only in
    // a match with an AI). A board change, with a Goals Changed event and the
    // usual win check.
    UFUNCTION(BlueprintCallable, Category = "Level|Changes")
    void SetLoseConditions(EConnectItRole Side, const TArray<UConnectIt_MatchConditionAsset*>& Conditions,
        EConnectItChangeTiming Timing = EConnectItChangeTiming::Immediately);

    // Moves the level to Phase (a ConnectIt.Phase.* tag). A board change: the
    // phase is recorded on the board (clients and UI read it with the
    // GameState's Get Current Phase), a Phase Changed event plays, and once it
    // is committed every director hears On Phase Changed. At a checkpoint, the
    // usual order is Begin Phase, then Set Win / Lose Conditions, then Set
    // Layout -- they play in the order asked.
    UFUNCTION(BlueprintCallable, Category = "Level|Changes")
    void BeginPhase(UPARAM(meta = (Categories = "ConnectIt.Phase")) FGameplayTag Phase,
        EConnectItChangeTiming Timing = EConnectItChangeTiming::Immediately);

    // Changes the board's tiles to Layout, as one board change: tiles not in
    // it go (with their pieces), new ones come (with their starting values;
    // a starting piece can score), shared ones keep their piece and
    // multiplier; then the camera moves to the layout's, if it has one. One
    // win check, on the final board. Worked out against the board when it is
    // applied, so a layout queued behind other changes still lands right.
    UFUNCTION(BlueprintCallable, Category = "Level|Changes")
    void SetLayout(UConnectIt_BoardLayout* Layout,
        EConnectItChangeTiming Timing = EConnectItChangeTiming::Immediately);

    // The match's rules as they are now (a copy) -- including each side's
    // current goals (Player / AI Win Groups)
    UFUNCTION(BlueprintPure, Category = "Level|Changes")
    FConnectItRuleSet GetMatchRules() const;

private:

    // Indices into Triggers that have fired and not been re-armed
    TSet<int32> FiredTriggers;

    TArray<TObjectPtr<UConnectIt_MatchConditionAsset>> GetTriggerConditions() const;
};
