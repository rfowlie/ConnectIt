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
class UConnectIt_AIDirector;
class UConnectIt_AICoordinator;


// The ConnectIt AI opponent. Owns turn timing and submitting; WHO it is --
// what it may do (a loadout), what proposes its moves (strategies) and how
// one is picked (a selector) -- comes from the match preset's AI director,
// so this controller never changes per level.
// Server-only -- AI controllers never exist on clients.
//
// Handles its own turn directly (no actions component -- that machinery is
// for a human's input-driven action stack): turn notifications from
// ParticipantComponent start/cancel decisions, the match phase pauses them,
// ATurnBasedPlayerState::CanEndTurn + ServerSubmitTurnEnd end the turn.
//
// Turn start -> BeginDecision (reads the board and rules, game thread) ->
// Coordinator->Begin (the director's strategies, then its selector) -> ... ->
// HandleCoordinatorConcluded (stale check, minimum think time) ->
// SubmitDecision (each candidate in order as the loadout action producing its
// operation tag, straight to the GameMode's Mediator, until one is accepted)
// -> end turn, or decide again if the turn-end requirements aren't met yet.
UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API AConnectIt_AIController : public ATurnBasedAIController
{
    GENERATED_BODY()

public:

    explicit AConnectIt_AIController(
        const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // This AI's own copy of the match preset's director
    UFUNCTION(BlueprintPure, Category = "ConnectIt|AI")
    UConnectIt_AIDirector* GetDirector() const { return Director; }

    // A new loadout for this AI (what the director's Set AI Loadout calls).
    // During its own turn the change waits until the turn ends, so it can't
    // hand itself extra moves mid-turn.
    void ChangeLoadout(UActionLoadoutDataAsset* Loadout);

    // The coordinator's picked candidates, best first. Called by
    // UConnectIt_AICoordinator only.
    void HandleCoordinatorConcluded(const FConnectItAIDecision& Decision);

protected:

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

    // Duplicates the match preset's AI director (or a default one), seeds the
    // PlayerState from its starting loadout, tells it the match started.
    void InitialiseFromMatchSetup();

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

    // Snapshots the board and rules into a context and starts the
    // coordinator with the director's strategies and selector. Called from
    // OnMyTurnStarted, then again from SubmitDecision for as long as this
    // turn's requirements aren't satisfied (a loadout needing more than one
    // move per turn).
    void BeginDecision();

    // BeginDecision once the board has settled (AConnectIt_GameMode::
    // IsBoardSettled): a decision is only ever made from the board its move
    // will be applied to. Now if it already has; otherwise when the GameMode
    // says it has.
    void BeginDecisionWhenSettled();

    // AConnectIt_GameMode::OnBoardChangeSettled
    void HandleBoardChangeSettled();

    // Re-checks the decision is still current, then submits it -- or, if the
    // board changed or isn't settled since it was made, decides again once
    // it is
    void SubmitDecision(int32 DecisionId, FConnectItAIDecision Decision);

    // Submits one candidate as the loadout action that produces its kind of
    // operation. True if the server accepted it.
    bool SubmitCandidate(const FConnectItAICandidate& Candidate, int32 Index);

    // True while DecisionId is the latest decision, and it is still this
    // controller's turn in the turn it was started on.
    bool IsDecisionStillCurrent(int32 DecisionId) const;

    // Stops the decision in progress and any pending think timer; any late
    // result is dropped.
    void CancelDecision();

    // Runs each decision (strategies + selector)
    UPROPERTY()
    TObjectPtr<UConnectIt_AICoordinator> Coordinator = nullptr;

    // Set by ChangeLoadout during the AI's own turn; applied when it ends
    UPROPERTY()
    TObjectPtr<UActionLoadoutDataAsset> PendingLoadout = nullptr;
    bool bHasPendingLoadout = false;

    // This controller's own AI director, made from the match preset's class
    // (or the default one)
    UPROPERTY()
    TObjectPtr<UConnectIt_AIDirector> Director = nullptr;

    // Between this AI's turn start and its end
    bool bTurnInProgress = false;

    void OnMyTurnEnded(int32 TurnNumber);

    FDelegateHandle TurnNotificationHandle;
    FDelegateHandle MatchPhaseHandle;
    FDelegateHandle BoardSettledHandle;

    // A decision is waiting for the board to settle (BeginDecisionWhenSettled)
    bool bWaitingForSettledBoard = false;

    // AConnectIt_GameMode::GetNumBoardChanges when the decision was started:
    // a different count at submit means the board changed underneath it
    int32 DecisionBoardChanges = INDEX_NONE;

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
