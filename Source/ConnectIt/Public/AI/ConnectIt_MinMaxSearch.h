// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AI/ConnectIt_AIStrategy.h"
#include "MinMax/ConnectIt_MinMaxRules.h"
#include "MinMax/ConnectIt_MinMaxWeights.h"
#include "Search/GI_SearchAsync.h"
#include "ConnectIt_MinMaxSearch.generated.h"

// The MinMax search as a tool any AI strategy can use -- the built-in MinMax
// strategy is one caller, a Blueprint strategy's "Find Best Moves (MinMax)"
// node another. It looks ahead MaxDepth moves (both sides) on a background
// thread and ranks the moves available now.


// How to search. Every value can be changed per call -- e.g. search deeper,
// or with more moves, when the AI is losing.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItMinMaxSearchSettings
{
    GENERATED_BODY()

    // Deepest look-ahead, in moves (one move = one placement by either side)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MinMax", meta = (ClampMin = 1, ClampMax = 10))
    int32 MaxDepth = 4;

    // Search time when the decision gives none (e.g. the Blueprint node
    // called with a context that has no budget). Under an AI director the
    // director's think time is used instead. When it runs out the search
    // stops deepening and ranks moves from the deepest look-ahead it
    // completed. 0 = no limit (always reaches MaxDepth).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MinMax", meta = (ClampMin = 0.0, Units = "s"))
    float DefaultTimeBudgetSeconds = 1.5f;

    // What the search values in an unfinished position
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MinMax")
    FConnectItMinMaxEvaluationWeights EvaluationWeights;

    // Which moves it tries first (speed only)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MinMax")
    FConnectItMinMaxOrderingWeights OrderingWeights;

    // The kinds of move (request types) the searching side may use. Empty =
    // what it may do right now (the decision context's AvailableRequestTypes).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MinMax")
    FGameplayTagContainer OwnRequestTypes;

    // The kinds of move the search assumes the opponent may reply with.
    // Empty = whatever the opponent's loadout grants.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MinMax")
    FGameplayTagContainer OpponentRequestTypes;
};

// One move the search ranked
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItMinMaxRankedMove
{
    GENERATED_BODY()

    // The board operation (e.g. Place Piece) -- ready to be a decision candidate
    UPROPERTY(BlueprintReadOnly, Category = "MinMax")
    FInstancedStruct Operation;

    // How good the move is for the searching side (higher = better). A
    // score near +/- 10,000,000 is a forced win / loss.
    UPROPERTY(BlueprintReadOnly, Category = "MinMax")
    int32 Score = 0;
};

USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItMinMaxSearchResult
{
    GENERATED_BODY()

    // Every move available now, best first
    UPROPERTY(BlueprintReadOnly, Category = "MinMax")
    TArray<FConnectItMinMaxRankedMove> RankedMoves;

    // How far ahead it looked, and how much work that took
    UPROPERTY(BlueprintReadOnly, Category = "MinMax")
    int32 DepthReached = 0;

    UPROPERTY(BlueprintReadOnly, Category = "MinMax")
    int64 NodesVisited = 0;

    UPROPERTY(BlueprintReadOnly, Category = "MinMax")
    float ElapsedSeconds = 0.f;

    // The time budget ran out before MaxDepth
    UPROPERTY(BlueprintReadOnly, Category = "MinMax")
    bool bOutOfTime = false;

    // One line for logs and decision summaries
    FString Describe() const;
};

// The best-so-far ranking of a running search: the search thread publishes
// each depth it completes, and the game thread reads the latest whenever it
// likes -- e.g. when a strategy is asked for its answer before its search
// has finished. Shared between the two through FConnectItMinMaxSearchProgressPtr.
class CONNECTIT_API FConnectItMinMaxSearchProgress
{
public:

    using FScoredMoves = TArray<GameIntelligence::Search::MinMax::TScoredMove<FConnectItMinMaxRules::FMove>>;

    // Search thread, after each completed depth
    void Publish(const FScoredMoves& RootScores, int32 Depth);

    // The latest completed depth's ranking, best first. Empty (and depth 0)
    // until depth 1 completes.
    TArray<FConnectItMinMaxRankedMove> GetRankedMoves(int32* OutDepthReached = nullptr) const;

    int32 GetDepthReached() const;

private:

    mutable FCriticalSection Lock;
    FScoredMoves RootScores;
    int32 DepthReached = 0;
};

using FConnectItMinMaxSearchProgressPtr = TSharedPtr<FConnectItMinMaxSearchProgress, ESPMode::ThreadSafe>;

// The search, in three steps so tests can run it synchronously:
// Prepare (game thread) -> FConnectItMinMaxSearch::Run (any thread) ->
// ToResult (game thread). Launch does all three asynchronously.
namespace ConnectItMinMaxSearch
{
    struct FPrepared
    {
        TSharedPtr<const FConnectItMinMaxRules, ESPMode::ThreadSafe> Rules;
        FConnectItMinMaxRules::FState Root;
        GameIntelligence::Search::MinMax::FParams Params;
    };

    // The kinds of move the search can model at all (today: Place Piece)
    CONNECTIT_API FGameplayTagContainer GetModelledRequestTypes();

    // Builds everything the search reads, as plain data. False (with a
    // reason) if it can't search: not two factions, or no move the search can
    // model for the searching side.
    CONNECTIT_API bool Prepare(
        const FConnectItAIDecisionContext& Context,
        const FConnectItMinMaxSearchSettings& Settings,
        FPrepared& OutPrepared,
        FString* OutWhyNot = nullptr);

    CONNECTIT_API FConnectItMinMaxSearchResult ToResult(const FConnectItMinMaxSearch::FResult& Raw);

    // The search's moves as board operations, in the order given
    CONNECTIT_API TArray<FConnectItMinMaxRankedMove> ToRankedMoves(
        const FConnectItMinMaxSearchProgress::FScoredMoves& RootScores);

    // A confidence (0..1) per ranked move, comparable across strategies: a
    // forced win is 1 and a forced loss 0; the rest are placed between the
    // worst (0) and best (1) of the other scores. All equal, or a single
    // move, is 1.
    CONNECTIT_API TArray<float> ComputeConfidences(const TArray<FConnectItMinMaxRankedMove>& RankedMoves);

    // The ranked moves (best first) as a decision of at most MaxCandidates:
    // each with its score as StrategyScore, a confidence, and a summary
    // starting with SearchDescription.
    CONNECTIT_API FConnectItAIDecision MakeDecision(
        const TArray<FConnectItMinMaxRankedMove>& RankedMoves,
        const FString& SearchDescription,
        int32 MaxCandidates);

    // Prepare, then search on a background task; OnComplete runs on the game
    // thread (not if Prepare failed -- the return value says so). Setting
    // CancelFlag stops it; OnComplete then gets bCancelled = true. Progress,
    // if given, receives each completed depth's ranking as the search runs.
    CONNECTIT_API bool Launch(
        const FConnectItAIDecisionContext& Context,
        const FConnectItMinMaxSearchSettings& Settings,
        GameIntelligence::Search::FCancelFlag CancelFlag,
        TUniqueFunction<void(FConnectItMinMaxSearchResult&& Result, bool bCancelled)> OnComplete,
        FString* OutWhyNot = nullptr,
        FConnectItMinMaxSearchProgressPtr Progress = nullptr);
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnConnectItMinMaxSearchFinished, const FConnectItMinMaxSearchResult&, Result);

// Blueprint node: "Find Best Moves (MinMax)". Searches on a background
// thread; On Complete fires on the game thread with the ranked moves. On
// Failed fires if the search can't run (not two factions, no move it can
// model) or is cancelled.
UCLASS()
class CONNECTIT_API UConnectIt_FindBestMovesAsync : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:

    // Context: the decision context a strategy was given (board, rules,
    // whose turn, available moves). Settings: how to search.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|AI",
        meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject",
            DisplayName = "Find Best Moves (MinMax)"))
    static UConnectIt_FindBestMovesAsync* FindBestMoves(
        UObject* WorldContextObject,
        const FConnectItAIDecisionContext& Context,
        const FConnectItMinMaxSearchSettings& Settings);

    // Turns the best moves into a decision (best first, at most
    // MaxCandidates), with each move's score and a confidence
    UFUNCTION(BlueprintPure, Category = "ConnectIt|AI")
    static FConnectItAIDecision MakeDecisionFromSearch(const FConnectItMinMaxSearchResult& Result, int32 MaxCandidates = 5);

    // Stops the search; On Failed fires
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|AI")
    void Cancel();

    UPROPERTY(BlueprintAssignable)
    FOnConnectItMinMaxSearchFinished OnComplete;

    UPROPERTY(BlueprintAssignable)
    FOnConnectItMinMaxSearchFinished OnFailed;

    virtual void Activate() override;

private:

    FConnectItAIDecisionContext Context;
    FConnectItMinMaxSearchSettings Settings;
    // Shared with the running search task; null until Activate
    TSharedPtr<std::atomic<bool>, ESPMode::ThreadSafe> CancelFlag;
    bool bFinished = false;

    void Finish(const FConnectItMinMaxSearchResult& Result, bool bSucceeded);
};
