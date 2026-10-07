// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Controller/TurnBasedAIController.h"
#include "AI/ConnectIt_AIStrategy.h"
#include "Engine/TimerHandle.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedMechanicsStructs.h"
#include "ConnectIt_AIController.generated.h"

class UActionLoadoutDataAsset;
class UConnectIt_AIProfile;


// The ConnectIt AI opponent. Owns turn timing and submitting; WHO it is --
// how it picks a move (a UConnectIt_AIStrategy, e.g. MinMax for Classic, or a
// bespoke strategy) and what it may do (a loadout) -- comes from a
// UConnectIt_AIProfile, so this controller never changes per level.
// Server-only -- AI controllers never exist on clients.
//
// Handles its own turn directly (no actions component -- that machinery is
// for a human's input-driven action stack): turn notifications from
// ParticipantComponent start/cancel decisions, the match phase pauses them,
// ATurnBasedPlayerState::CanEndTurn + ServerSubmitTurnEnd end the turn.
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

    explicit AConnectIt_AIController(
        const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    UFUNCTION(BlueprintPure, Category = "ConnectIt|AI")
    UConnectIt_AIStrategy* GetStrategy() const { return Strategy; }

protected:

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

    void InitialiseFromLevelConfig();

    // Which AI profile this match uses: the main-menu match setup's choice
    // for this level if there is one, otherwise the level config's default.
    const UConnectIt_AIProfile* ResolveAIProfile() const;

    // Seeds the PlayerState's action state (uses, caps, turn-end tree) from
    // the AI's loadout -- what the GameMode does for humans in PostLogin.
    void SeedActionState(UActionLoadoutDataAsset* Loadout);

    // ParticipantComponent turn notifications (server-local for an AI):
    // TurnStart/TurnActive start a decision once per turn number; turn
    // end/timeout/skip cancel it.
    void HandleTurnNotification(const FTurnNotification& Notification);

    // GameState match phase: pause/game over cancel a decision in progress;
    // resuming re-decides if it is still this AI's turn.
    void HandleMatchPhaseChanged(EMatchPhase NewPhase);

    void OnMyTurnStarted();

    // It is this controller's turn right now (per the GameState)
    bool IsMyTurnNow() const;

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

    // This controller's own copy of the profile's strategy template
    UPROPERTY()
    TObjectPtr<UConnectIt_AIStrategy> Strategy = nullptr;

    FDelegateHandle TurnNotificationHandle;
    FDelegateHandle MatchPhaseHandle;

    // The turn number a decision was last started for -- the participant
    // component notifies both TurnStart and TurnActive, start only once.
    int32 LastStartedTurnNumber = INDEX_NONE;

    // A pause interrupted this AI's decision; re-decide when play resumes
    bool bResumeOnUnpause = false;

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
