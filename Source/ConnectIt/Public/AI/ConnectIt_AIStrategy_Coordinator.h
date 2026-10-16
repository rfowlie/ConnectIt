// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/ConnectIt_AIStrategy.h"
#include "ConnectIt_AIStrategy_Coordinator.generated.h"

// One child strategy's answer, as the coordinator's judge sees it
USTRUCT(BlueprintType)
struct FConnectItAIChildDecision
{
    GENERATED_BODY()

    // Position in the coordinator's Children list (0 = highest priority)
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    int32 ChildIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "AI")
    TObjectPtr<UConnectIt_AIStrategy> Child = nullptr;

    // False if the child hadn't answered when the time budget ran out
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    bool bAnswered = false;

    UPROPERTY(BlueprintReadOnly, Category = "AI")
    FConnectItAIDecision Decision;
};

// A strategy made of strategies. It asks every child at once, collects their
// candidate lists until its time budget runs out (or every child has
// answered), then ranks all candidates with RankCandidates and answers with
// the best -- like any other strategy, so coordinators can contain
// coordinators.
//
// Children are plain strategies (MinMax, a Blueprint strategy...), in priority
// order. Each gets the same decision context, with a share of the
// coordinator's time budget. A child still working when the budget runs out
// is cancelled. If no child has answered by then, the coordinator waits for
// the first answer.
//
// The default judge is child priority order: the first child's candidates
// first, then the next child's, with repeats removed. Override
// RankCandidates (C++ or Blueprint) for anything else -- see the recipe
// "Combine AI strategies" for weighted random and re-scoring by simulation.
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced,
    meta = (DisplayName = "Coordinator"))
class CONNECTIT_API UConnectIt_AIStrategy_Coordinator : public UConnectIt_AIStrategy
{
    GENERATED_BODY()

public:

    // The strategies to ask, highest priority first
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "AI|Coordinator")
    TArray<TObjectPtr<UConnectIt_AIStrategy>> Children;

    // How long to collect answers. 0 = wait for every child.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Coordinator",
        meta = (ClampMin = 0.0, Units = "s"))
    float TimeBudgetSeconds = 2.f;

    // The share of TimeBudgetSeconds each child is told it has (a search
    // stops deepening when its own budget runs out), leaving the rest for
    // the coordinator to judge
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Coordinator",
        meta = (ClampMin = 0.1, ClampMax = 1.0))
    float ChildBudgetShare = 0.8f;

    // What its children can play, together
    virtual FGameplayTagContainer GetSupportedRequestTypes_Implementation() const override;

    // Ends the collection now, as the time budget running out does. Called
    // by the coordinator's own timer; tests call it directly.
    void EndTimeBudget();

protected:

    virtual void BeginDecision_Implementation(const FConnectItAIDecisionContext& Context) override;
    virtual void CancelDecision_Implementation() override;

    // The judge: all children's answers in, one ranked decision out (the
    // coordinator keeps the best MaxCandidates). Default: child priority
    // order, repeats removed.
    UFUNCTION(BlueprintNativeEvent, Category = "AI|Coordinator")
    FConnectItAIDecision RankCandidates(
        const FConnectItAIDecisionContext& Context, const TArray<FConnectItAIChildDecision>& ChildDecisions);

private:

    void HandleChildFinished(const FConnectItAIDecision& Decision, int32 ChildIndex);
    void Conclude();
    void StopChildren();

    // The decision being collected
    FConnectItAIDecisionContext ActiveContext;
    TArray<FConnectItAIChildDecision> Collected;
    int32 CollectingDecisionId = INDEX_NONE;
    bool bBudgetEnded = false;
    FTimerHandle BudgetTimer;
};
