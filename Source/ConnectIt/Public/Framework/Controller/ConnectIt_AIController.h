// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Controller/TurnBasedAIController.h"
#include "AI/ConnectIt_AIStrategy.h"
#include "Engine/TimerHandle.h"
#include "ConnectIt_AIController.generated.h"


// The ConnectIt AI opponent. Owns turn timing and submitting; HOW it picks a
// move is the level's UConnectIt_AIStrategy (e.g. MinMax for Classic, or a
// bespoke strategy), so this controller never changes per level.
// Server-only -- AI controllers never exist on clients.
//
// Turn start -> BeginDecision (reads the board and rules, game thread) ->
// Strategy->StartDecision -> ... -> HandleDecisionFinished (stale check,
// minimum think time) -> SubmitDecision (as the loadout action producing the
// decision's request type, straight to the GameMode's Mediator) -> end turn,
// or decide again if the turn-end requirements aren't met yet.
UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API AConnectIt_AIController : public ATurnBasedAIController
{
    GENERATED_BODY()

public:

    // Installs UConnectIt_AIActionsComponent in place of the base's hardcoded
    // UTurnBasedActionsComponent, the same FObjectInitializer::
    // SetDefaultSubobjectClass technique AConnectIt_PlayerController already
    // uses for its own sibling component -- that subclass is this
    // controller's "my turn started" hook (see its own class comment).
    explicit AConnectIt_AIController(
        const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // Called by UConnectIt_AIActionsComponent when this AI's turn starts --
    // the AI-side equivalent of a human clicking a tile.
    void OnMyTurnStarted();

    UFUNCTION(BlueprintPure, Category = "ConnectIt|AI")
    UConnectIt_AIStrategy* GetStrategy() const { return Strategy; }

protected:

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

    void InitialiseFromLevelConfig();

    // Adventure-mode hook -- checks blackboard for a forced move. Currently
    // always false; Classic ConnectIt has no forced moves.
    bool CheckAndApplyForcedMove();

    // Snapshots the board and rules into a context and hands it to the
    // strategy. Called from OnMyTurnStarted, then again from SubmitDecision
    // for as long as this turn's requirements aren't satisfied (a loadout
    // needing more than one move per turn).
    void BeginDecision();

    // Bound to Strategy->OnDecisionFinished
    void HandleDecisionFinished(const FConnectItAIDecision& Decision);

    // Re-checks the decision is still current, then submits it
    void SubmitDecision(int32 DecisionId, FConnectItAIDecision Decision);

    // True while DecisionId is the latest decision, and it is still this
    // controller's turn in the turn it was started on.
    bool IsDecisionStillCurrent(int32 DecisionId) const;

    // Stops the strategy's decision in progress and any pending think timer;
    // any late result is dropped.
    void CancelDecision();

    // This controller's own copy of the level's AIStrategy template
    UPROPERTY()
    TObjectPtr<UConnectIt_AIStrategy> Strategy = nullptr;

    // Increments per decision so a late result from an earlier one is ignored
    int32 CurrentDecisionId = 0;
    int32 DecisionTurnNumber = INDEX_NONE;
    double DecisionStartTime = 0.0;

    FTimerHandle ThinkTimerHandle;

    // Safety cap on BeginDecision/SubmitDecision round-trips within one turn
    // -- stops the AI looping forever if the loadout's turn-end requirements
    // can never be satisfied. Reset in OnMyTurnStarted.
    int32 DecisionsThisTurn = 0;
    static constexpr int32 MaxDecisionsPerTurn = 16;
};
