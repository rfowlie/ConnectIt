// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/TimerHandle.h"
#include "AI/ConnectIt_AIStrategy.h"
#include "ConnectIt_AICoordinator.generated.h"

class UConnectIt_AISelector;


// Runs one AI decision: starts every strategy the director lists, collects
// their answers within the think-time budget, and has the director's selector
// pick. Fixed machinery, owned by AConnectIt_AIController -- nothing about it
// is configured; what it runs comes from the AI director.
//
//   Begin -> every strategy starts at once (the list order is their
//   priority), each told it has Budget x StrategyBudgetShare -> polled on a
//   short timer -> concludes when all have finished, or when the budget runs
//   out (then each strategy's best answer so far is used; nothing at all yet
//   = keep waiting for the first, with a warning naming the slow ones) ->
//   strategies still working are stopped -> Selector->Select -> the
//   controller's HandleCoordinatorConcluded.
//
// Polls rather than listening: a strategy only has to keep its best answer
// (UConnectIt_AIStrategy::GetBestSoFar), nothing calls back.
UCLASS()
class CONNECTIT_API UConnectIt_AICoordinator : public UObject
{
    GENERATED_BODY()

public:

    // Starts a decision (stopping any still running). Budget 0 = wait for
    // every strategy to finish. Null strategies are skipped; no selector =
    // first available.
    void Begin(
        const FConnectItAIDecisionContext& Context,
        const TArray<UConnectIt_AIStrategy*>& Strategies,
        UConnectIt_AISelector* Selector,
        float TimeBudgetSeconds,
        float StrategyBudgetShare);

    // Stops the decision in progress and its strategies; no answer follows
    void Cancel();

    bool IsRunning() const { return bRunning; }

    // The last concluded decision
    const FConnectItAIDecision& GetLastDecision() const { return LastDecision; }

    // Checks the strategies; concludes if they're all done (or the budget is
    // spent and one has a move). The poll timer calls it; public for tests,
    // which have no world to run timers in.
    void Poll();

    // The budget has run out. Normally the budget timer; public for tests.
    void EndTimeBudget();

    virtual UWorld* GetWorld() const override;

private:

    static constexpr float PollIntervalSeconds = 0.05f;

    void Conclude();
    void ClearTimers();
    TArray<FString> GetUnfinishedLabels() const;

    UPROPERTY()
    TArray<TObjectPtr<UConnectIt_AIStrategy>> ActiveStrategies;

    UPROPERTY()
    TObjectPtr<UConnectIt_AISelector> ActiveSelector = nullptr;

    // Used when no selector is given
    UPROPERTY()
    TObjectPtr<UConnectIt_AISelector> FallbackSelector = nullptr;

    FConnectItAIDecisionContext ActiveContext;
    FConnectItAIDecision LastDecision;
    bool bRunning = false;
    // Counts Begin calls, to tell this decision from one started meanwhile
    int32 DecisionSerial = 0;
    bool bBudgetEnded = false;
    bool bWarnedNothingYet = false;

    FTimerHandle PollTimer;
    FTimerHandle BudgetTimer;
};
