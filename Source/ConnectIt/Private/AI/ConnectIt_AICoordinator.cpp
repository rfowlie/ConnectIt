// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_AICoordinator.h"
#include "AI/ConnectIt_AISelector.h"
#include "Framework/Controller/ConnectIt_AIController.h"
#include "Engine/World.h"
#include "TimerManager.h"


void UConnectIt_AICoordinator::Begin(
    const FConnectItAIDecisionContext& Context,
    const TArray<UConnectIt_AIStrategy*>& Strategies,
    UConnectIt_AISelector* Selector,
    float TimeBudgetSeconds,
    float StrategyBudgetShare)
{
    Cancel();

    ActiveContext = Context;
    ActiveSelector = Selector;
    ActiveStrategies.Reset();
    for (UConnectIt_AIStrategy* Strategy : Strategies)
    {
        if (IsValid(Strategy)) ActiveStrategies.Add(Strategy);
    }
    bRunning = true;
    ++DecisionSerial;
    bBudgetEnded = false;
    bWarnedNothingYet = false;

    if (ActiveStrategies.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectIt_AICoordinator: the AI director has no strategies -- no move"));
        Conclude();
        return;
    }

    // Each strategy is told it has a share of the budget, leaving the rest
    // for collecting and selecting
    FConnectItAIDecisionContext StrategyContext = Context;
    if (TimeBudgetSeconds > 0.f)
    {
        const float StrategyBudget = TimeBudgetSeconds * FMath::Clamp(StrategyBudgetShare, 0.01f, 1.f);
        StrategyContext.TimeBudgetSeconds = Context.TimeBudgetSeconds > 0.f
            ? FMath::Min(Context.TimeBudgetSeconds, StrategyBudget) : StrategyBudget;
    }

    // Timers before the strategies start (Poll below may conclude at once)
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(PollTimer,
            FTimerDelegate::CreateUObject(this, &UConnectIt_AICoordinator::Poll), PollIntervalSeconds, true);
        if (TimeBudgetSeconds > 0.f)
        {
            World->GetTimerManager().SetTimer(BudgetTimer,
                FTimerDelegate::CreateUObject(this, &UConnectIt_AICoordinator::EndTimeBudget), TimeBudgetSeconds, false);
        }
    }

    for (UConnectIt_AIStrategy* Strategy : ActiveStrategies)
    {
        Strategy->StartDecision(StrategyContext);
    }

    // Strategies that answer at once may already be done. Nothing after
    // this: concluding hands control to the controller, which may Begin the
    // next decision before this returns.
    Poll();
}

void UConnectIt_AICoordinator::Cancel()
{
    if (!bRunning) return;

    bRunning = false;
    ClearTimers();
    for (UConnectIt_AIStrategy* Strategy : ActiveStrategies)
    {
        if (IsValid(Strategy)) Strategy->StopDecision();
    }
}

void UConnectIt_AICoordinator::Poll()
{
    if (!bRunning) return;

    const bool bAllFinished = !ActiveStrategies.ContainsByPredicate(
        [](const UConnectIt_AIStrategy* Strategy) { return IsValid(Strategy) && !Strategy->IsDecisionFinished(); });
    if (bAllFinished)
    {
        Conclude();
        return;
    }

    if (bBudgetEnded)
    {
        const bool bAnyMove = ActiveStrategies.ContainsByPredicate(
            [](const UConnectIt_AIStrategy* Strategy) { return IsValid(Strategy) && Strategy->GetBestSoFar().HasMove(); });
        if (bAnyMove)
        {
            Conclude();
        }
    }
}

void UConnectIt_AICoordinator::EndTimeBudget()
{
    if (!bRunning) return;
    bBudgetEnded = true;

    // Concluding may start the controller's next decision before Poll returns
    const int32 Serial = DecisionSerial;
    Poll();

    // Still nothing to play: keep polling for the first answer
    if (bRunning && Serial == DecisionSerial && !bWarnedNothingYet)
    {
        bWarnedNothingYet = true;
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_AICoordinator: think time is up but no strategy has a move yet -- waiting for the "
                 "first one. Still working: %s"),
            *FString::Join(GetUnfinishedLabels(), TEXT(", ")));
    }
}

void UConnectIt_AICoordinator::Conclude()
{
    // Collect before stopping anyone: a stopped search still has its best so far
    TArray<FConnectItAIStrategyAnswer> Answers;
    for (int32 Index = 0; Index < ActiveStrategies.Num(); Index++)
    {
        const UConnectIt_AIStrategy* Strategy = ActiveStrategies[Index];
        if (!IsValid(Strategy)) continue;

        FConnectItAIStrategyAnswer& Answer = Answers.AddDefaulted_GetRef();
        Answer.StrategyIndex = Index;
        Answer.SourceLabel = Strategy->GetSourceLabel();
        Answer.bFinished = Strategy->IsDecisionFinished();
        Answer.Decision = Strategy->GetBestSoFar();
    }

    const TArray<FString> CutOff = GetUnfinishedLabels();
    if (!CutOff.IsEmpty())
    {
        UE_LOG(LogTemp, Log, TEXT("ConnectIt_AICoordinator: out of think time -- using the best so far of: %s"),
            *FString::Join(CutOff, TEXT(", ")));
    }

    bRunning = false;
    ClearTimers();
    for (UConnectIt_AIStrategy* Strategy : ActiveStrategies)
    {
        if (IsValid(Strategy)) Strategy->StopDecision();
    }

    UConnectIt_AISelector* Selector = ActiveSelector;
    if (!IsValid(Selector))
    {
        if (!IsValid(FallbackSelector))
        {
            FallbackSelector = NewObject<UConnectIt_AISelector_FirstAvailable>(this);
        }
        Selector = FallbackSelector;
    }
    LastDecision = Selector->Select(ActiveContext, Answers);

    // Last: the controller may start the next decision from here
    if (AConnectIt_AIController* Controller = GetTypedOuter<AConnectIt_AIController>())
    {
        Controller->HandleCoordinatorConcluded(LastDecision);
    }
}

void UConnectIt_AICoordinator::ClearTimers()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PollTimer);
        World->GetTimerManager().ClearTimer(BudgetTimer);
    }
}

TArray<FString> UConnectIt_AICoordinator::GetUnfinishedLabels() const
{
    TArray<FString> Labels;
    for (const UConnectIt_AIStrategy* Strategy : ActiveStrategies)
    {
        if (IsValid(Strategy) && !Strategy->IsDecisionFinished())
        {
            Labels.Add(Strategy->GetSourceLabel().ToString());
        }
    }
    return Labels;
}

UWorld* UConnectIt_AICoordinator::GetWorld() const
{
    if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)) return nullptr;
    const UObject* Outer = GetOuter();
    return Outer ? Outer->GetWorld() : nullptr;
}
