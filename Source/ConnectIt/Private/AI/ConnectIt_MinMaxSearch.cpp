// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_MinMaxSearch.h"
#include "ConnectIt_GameplayTags.h"


namespace MinMax = GameIntelligence::Search::MinMax;

FString FConnectItMinMaxSearchResult::Describe() const
{
    return FString::Printf(TEXT("MinMax depth %d%s, %lld nodes in %.0f ms"),
        DepthReached, bOutOfTime ? TEXT(" (out of time)") : TEXT(""),
        NodesVisited, ElapsedSeconds * 1000.0);
}

void FConnectItMinMaxSearchProgress::Publish(const FScoredMoves& InRootScores, int32 Depth)
{
    FScopeLock ScopeLock(&Lock);
    RootScores = InRootScores;
    DepthReached = Depth;
}

TArray<FConnectItMinMaxRankedMove> FConnectItMinMaxSearchProgress::GetRankedMoves(int32* OutDepthReached) const
{
    // Copy under the lock, convert outside it: the search thread only ever
    // waits for the copy
    FScoredMoves Snapshot;
    {
        FScopeLock ScopeLock(&Lock);
        Snapshot = RootScores;
        if (OutDepthReached)
        {
            *OutDepthReached = DepthReached;
        }
    }
    return ConnectItMinMaxSearch::ToRankedMoves(Snapshot);
}

int32 FConnectItMinMaxSearchProgress::GetDepthReached() const
{
    FScopeLock ScopeLock(&Lock);
    return DepthReached;
}

FGameplayTagContainer ConnectItMinMaxSearch::GetModelledOperationTags()
{
    // FConnectItMinMaxMove lists the operations the search can model; it
    // assumes every move ends the turn and has no use limit -- true of
    // placing a piece only. Widening this is the task "Explore letting MinMax
    // use moves other than PlacePiece".
    return FGameplayTagContainer(ConnectIt_BoardOperation_PlacePiece);
}

bool ConnectItMinMaxSearch::Prepare(
    const FConnectItAIDecisionContext& Context,
    const FConnectItMinMaxSearchSettings& Settings,
    FPrepared& OutPrepared,
    FString* OutWhyNot)
{
    // Negamax is a two-player search
    if (Context.Board.ScoreBoard.Num() != 2 || !Context.Board.ScoreBoard.IsValidIndex(Context.OwnSlot))
    {
        if (OutWhyNot)
        {
            *OutWhyNot = FString::Printf(TEXT("supports exactly 2 factions (board has %d, own slot %d)"),
                Context.Board.ScoreBoard.Num(), Context.OwnSlot);
        }
        return false;
    }

    const FGameplayTagContainer Modelled = GetModelledOperationTags();

    // Each side's kinds of move: the settings' if given, else what the
    // context says -- limited to what the search can model
    const FGameplayTagContainer& OwnSource =
        Settings.OwnOperationTags.IsEmpty() ? Context.AvailableOperationTags : Settings.OwnOperationTags;
    const FGameplayTagContainer OwnOperationTags = OwnSource.FilterExact(Modelled);

    FGameplayTagContainer OpponentOperationTags;
    if (Settings.OpponentOperationTags.IsEmpty())
    {
        for (const FGameplayTag& OperationTag : Modelled)
        {
            if (UConnectIt_AIStrategy::LoadoutGrantsOperation(Context.OpponentLoadout, OperationTag))
            {
                OpponentOperationTags.AddTag(OperationTag);
            }
        }
    }
    else
    {
        OpponentOperationTags = Settings.OpponentOperationTags.FilterExact(Modelled);
    }

    if (OwnOperationTags.IsEmpty())
    {
        if (OutWhyNot)
        {
            *OutWhyNot = TEXT("the searching side has no move the search can model (it needs Place Piece)");
        }
        return false;
    }

    if (Settings.EvaluationWeights.IsAllZero())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_MinMaxSearch: every evaluation weight is 0 -- every "
                 "non-winning position looks equal, only wins and losses count"));
    }

    const bool bOwnIsSlot0 = Context.OwnSlot == 0;
    OutPrepared.Rules = MakeShared<const FConnectItMinMaxRules, ESPMode::ThreadSafe>(
        Context.Board,
        Context.Rules,
        Settings.EvaluationWeights,
        Settings.OrderingWeights,
        bOwnIsSlot0 ? OwnOperationTags : OpponentOperationTags,
        bOwnIsSlot0 ? OpponentOperationTags : OwnOperationTags);

    OutPrepared.Root = OutPrepared.Rules->MakeRoot(Context.Board, Context.OwnSlot);
    OutPrepared.Params = MinMax::FParams();
    OutPrepared.Params.MaxDepth = FMath::Max(1, Settings.MaxDepth);
    // The decision's budget (the director's think time share) when it has
    // one, else the settings' default; 0 = no limit
    OutPrepared.Params.TimeBudgetSeconds = Context.TimeBudgetSeconds > 0.f
        ? Context.TimeBudgetSeconds
        : FMath::Max(0.f, Settings.DefaultTimeBudgetSeconds);
    return true;
}

FConnectItMinMaxSearchResult ConnectItMinMaxSearch::ToResult(const FConnectItMinMaxSearch::FResult& Raw)
{
    FConnectItMinMaxSearchResult Result;
    Result.DepthReached = Raw.DepthReached;
    Result.NodesVisited = Raw.NodesVisited;
    Result.ElapsedSeconds = static_cast<float>(Raw.ElapsedSeconds);
    Result.bOutOfTime = Raw.bOutOfTime;
    Result.RankedMoves = ToRankedMoves(Raw.RootScores);
    return Result;
}

TArray<FConnectItMinMaxRankedMove> ConnectItMinMaxSearch::ToRankedMoves(
    const FConnectItMinMaxSearchProgress::FScoredMoves& RootScores)
{
    TArray<FConnectItMinMaxRankedMove> RankedMoves;
    RankedMoves.Reserve(RootScores.Num());
    for (const auto& Scored : RootScores)
    {
        FConnectItMinMaxRankedMove& Ranked = RankedMoves.AddDefaulted_GetRef();
        Ranked.Score = Scored.Score;
        Visit([&Ranked](const auto& Operation)
        {
            Ranked.Operation = FInstancedStruct::Make(Operation);
        }, Scored.Move);
    }
    return RankedMoves;
}

TArray<float> ConnectItMinMaxSearch::ComputeConfidences(const TArray<FConnectItMinMaxRankedMove>& RankedMoves)
{
    const auto IsForced = [](int32 Score)
    {
        return FMath::Abs(Score) > FConnectItMinMaxRules::WinValue / 2;
    };

    // The range of the scores that aren't forced wins / losses
    int32 Best = TNumericLimits<int32>::Lowest();
    int32 Worst = TNumericLimits<int32>::Max();
    for (const FConnectItMinMaxRankedMove& Ranked : RankedMoves)
    {
        if (IsForced(Ranked.Score)) continue;
        Best = FMath::Max(Best, Ranked.Score);
        Worst = FMath::Min(Worst, Ranked.Score);
    }

    TArray<float> Confidences;
    Confidences.Reserve(RankedMoves.Num());
    for (const FConnectItMinMaxRankedMove& Ranked : RankedMoves)
    {
        if (IsForced(Ranked.Score))
        {
            Confidences.Add(Ranked.Score > 0 ? 1.f : 0.f);
        }
        else if (Best > Worst)
        {
            Confidences.Add(static_cast<float>(
                static_cast<double>(Ranked.Score - Worst) / static_cast<double>(Best - Worst)));
        }
        else
        {
            Confidences.Add(1.f);
        }
    }
    return Confidences;
}

FConnectItAIDecision ConnectItMinMaxSearch::MakeDecision(
    const TArray<FConnectItMinMaxRankedMove>& RankedMoves,
    const FString& SearchDescription,
    int32 MaxCandidates)
{
    const TArray<float> Confidences = ComputeConfidences(RankedMoves);

    FConnectItAIDecision Decision;
    const int32 Count = FMath::Min(FMath::Max(1, MaxCandidates), RankedMoves.Num());
    for (int32 Index = 0; Index < Count; Index++)
    {
        const FConnectItMinMaxRankedMove& Ranked = RankedMoves[Index];
        FConnectItAICandidate& Candidate = Decision.Candidates.AddDefaulted_GetRef();
        Candidate.Payload = Ranked.Operation;
        Candidate.Summary = FString::Printf(TEXT("%s -- rank %d of %d, score %d"),
            *SearchDescription, Index + 1, RankedMoves.Num(), Ranked.Score);
        Candidate.StrategyScore = static_cast<float>(Ranked.Score);
        Candidate.Confidence = Confidences[Index];
    }
    return Decision;
}

bool ConnectItMinMaxSearch::Launch(
    const FConnectItAIDecisionContext& Context,
    const FConnectItMinMaxSearchSettings& Settings,
    GameIntelligence::Search::FCancelFlag CancelFlag,
    TUniqueFunction<void(FConnectItMinMaxSearchResult&& Result, bool bCancelled)> OnComplete,
    FString* OutWhyNot,
    FConnectItMinMaxSearchProgressPtr Progress)
{
    FPrepared Prepared;
    if (!Prepare(Context, Settings, Prepared, OutWhyNot)) return false;

    FConnectItMinMaxSearch::FOnDepthCompleted OnDepthCompleted;
    if (Progress.IsValid())
    {
        // Runs on the search thread; the progress holder locks
        OnDepthCompleted = [Progress](const FConnectItMinMaxSearchProgress::FScoredMoves& RootScores, int32 Depth)
        {
            Progress->Publish(RootScores, Depth);
        };
    }

    MinMax::LaunchAlphaBetaAsync<FConnectItMinMaxRules>(
        Prepared.Rules.ToSharedRef(), MoveTemp(Prepared.Root), Prepared.Params, CancelFlag,
        [OnComplete = MoveTemp(OnComplete)](FConnectItMinMaxSearch::FResult&& Raw) mutable
        {
            const bool bCancelled = Raw.bCancelled;
            OnComplete(ToResult(Raw), bCancelled);
        },
        MoveTemp(OnDepthCompleted));
    return true;
}

// --- Blueprint node ---

UConnectIt_FindBestMovesAsync* UConnectIt_FindBestMovesAsync::FindBestMoves(
    UObject* WorldContextObject,
    const FConnectItAIDecisionContext& Context,
    const FConnectItMinMaxSearchSettings& Settings)
{
    UConnectIt_FindBestMovesAsync* Action = NewObject<UConnectIt_FindBestMovesAsync>();
    Action->Context = Context;
    Action->Settings = Settings;
    Action->RegisterWithGameInstance(WorldContextObject);
    return Action;
}

void UConnectIt_FindBestMovesAsync::Activate()
{
    const GameIntelligence::Search::FCancelFlag Flag = GameIntelligence::Search::MakeCancelFlag();
    CancelFlag = Flag;

    TWeakObjectPtr<UConnectIt_FindBestMovesAsync> WeakThis(this);
    FString WhyNot;
    const bool bLaunched = ConnectItMinMaxSearch::Launch(Context, Settings, Flag,
        [WeakThis](FConnectItMinMaxSearchResult&& Result, bool bCancelled)
        {
            if (UConnectIt_FindBestMovesAsync* Self = WeakThis.Get())
            {
                Self->Finish(Result, !bCancelled);
            }
        },
        &WhyNot);

    if (!bLaunched)
    {
        UE_LOG(LogTemp, Warning, TEXT("Find Best Moves (MinMax): can't search -- %s"), *WhyNot);
        Finish(FConnectItMinMaxSearchResult(), false);
    }
}

void UConnectIt_FindBestMovesAsync::Cancel()
{
    if (CancelFlag.IsValid())
    {
        CancelFlag->store(true);
    }
    Finish(FConnectItMinMaxSearchResult(), false);
}

void UConnectIt_FindBestMovesAsync::Finish(const FConnectItMinMaxSearchResult& Result, bool bSucceeded)
{
    if (bFinished) return;
    bFinished = true;

    if (bSucceeded)
    {
        OnComplete.Broadcast(Result);
    }
    else
    {
        OnFailed.Broadcast(Result);
    }
    SetReadyToDestroy();
}

FConnectItAIDecision UConnectIt_FindBestMovesAsync::MakeDecisionFromSearch(
    const FConnectItMinMaxSearchResult& Result, int32 MaxCandidates)
{
    return ConnectItMinMaxSearch::MakeDecision(Result.RankedMoves, Result.Describe(), MaxCandidates);
}
