// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "ConnectIt_Structs.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "ConnectIt_AIStrategy.generated.h"

class AConnectIt_AIController;
class UActionLoadoutDataAsset;


// Everything a strategy is given to decide one move, read from the live game
// on the game thread by AConnectIt_AIController.
USTRUCT(BlueprintType)
struct FConnectItAIDecisionContext
{
    GENERATED_BODY()

    // Copy of the current board
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    FConnectItBoardState Board;

    // The deciding AI's faction slot
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    int32 OwnSlot = INDEX_NONE;

    // This match's rules -- a copy of the GameMode's rule set (scoring, win
    // condition), including any per-match changes such as a
    // menu-chosen target score. Plain thread-safe data: a strategy may hand
    // it to a background search as is.
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    FConnectItRuleSet Rules;

    // Each side's loadout: the actions (and so the board operations) it has.
    // Ask UConnectIt_AIStrategy::LoadoutGrantsRequestType. Game thread only
    // -- unlike the rest of the context these are assets, not plain data, so
    // don't hand them to a background search. Opponent's is null if it has
    // no PlayerState.
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    TObjectPtr<const UActionLoadoutDataAsset> OwnLoadout;

    UPROPERTY(BlueprintReadOnly, Category = "AI")
    TObjectPtr<const UActionLoadoutDataAsset> OpponentLoadout;

    // The match's turn number this decision is for
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    int32 TurnNumber = 0;

    // Which move of this AI's turn is being decided: 0 for the first, 1 for
    // the second... (more than one when the loadout's turn-end requirements
    // need several moves; each is decided from the board the previous one
    // left). A strategy that remembers things between the moves of a turn
    // resets them when this is 0.
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    int32 DecisionIndexThisTurn = 0;

    // The kinds of move (request types, e.g. ConnectIt_Game_PlacePiece) this
    // AI may make RIGHT NOW: its loadout has an action that produces them, and
    // that action is usable this decision (per-turn cap, cooldown, uses left).
    // Offer only candidates of these types. The server still checks every
    // move; this is so a strategy doesn't have to work it out.
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    FGameplayTagContainer AvailableRequestTypes;

    // How long this decision may take, in seconds; 0 = no limit. Set by a
    // coordinator for its children; a strategy that searches should stop
    // when it runs out (the MinMax search does).
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    float TimeBudgetSeconds = 0.f;
};

// One move a strategy is willing to make
USTRUCT(BlueprintType)
struct FConnectItAICandidate
{
    GENERATED_BODY()

    // The board change itself: a FConnectItBoardOperation, e.g.
    // FConnectItBoardOperation_PlacePiece (wrap it with Make Instanced Struct)
    UPROPERTY(BlueprintReadWrite, Category = "AI")
    FInstancedStruct Payload;

    // Free text for the log -- why this move
    UPROPERTY(BlueprintReadWrite, Category = "AI")
    FString Summary;
};

// What a strategy decided: the moves it is willing to make, best first. The
// controller submits them in order -- each as the loadout action that
// produces that kind of operation, as its own faction -- until the server
// accepts one, the way a human tries again after a rejected move. If every
// candidate is rejected it logs an error and ends the turn. No candidates =
// no move.
USTRUCT(BlueprintType)
struct FConnectItAIDecision
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "AI")
    TArray<FConnectItAICandidate> Candidates;

    bool HasMove() const { return !Candidates.IsEmpty(); }
};

DECLARE_DELEGATE_OneParam(FOnConnectItAIDecisionFinished, const FConnectItAIDecision&);

// How an AI opponent picks its moves -- configured inline on a
// UConnectIt_AIProfile asset (the same inline-instanced pattern as the
// level's scoring / win / placeable rules).
// AConnectIt_AIController owns turn timing and submitting; the strategy only
// decides. Subclass in C++ (see UConnectIt_AIStrategy_MinMax) or Blueprint.
//
// Flow: the controller calls StartDecision -> BeginDecision runs; when the
// strategy has an answer (now, or later from an async task) it calls
// FinishDecision, which reaches the controller through OnDecisionFinished.
// StopDecision cancels: CancelDecision runs and any later FinishDecision is
// ignored.
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CONNECTIT_API UConnectIt_AIStrategy : public UObject
{
    GENERATED_BODY()

public:

    // The AI never moves sooner than this after it starts deciding, so a
    // fast decision still reads as "thinking". Applied by the controller.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI",
        meta = (ClampMin = 0.0, Units = "s"))
    float MinThinkSeconds = 0.6f;

    // At most this many candidates are kept from a decision (the best ones):
    // enough fallbacks for a rejected move, short enough to read in a log.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = 1))
    int32 MaxCandidates = 5;

    // Controller-facing entry points -- not overridable, they keep the
    // "is a decision active" bookkeeping honest around the hooks below.
    void StartDecision(const FConnectItAIDecisionContext& Context);
    void StopDecision();

    // Bound by the owning controller
    FOnConnectItAIDecisionFinished OnDecisionFinished;

    // The controller this strategy decides for (its outer -- the controller
    // duplicates the level's template with itself as outer). For reading
    // anything beyond the shared context, on the game thread.
    UFUNCTION(BlueprintPure, Category = "AI")
    AConnectIt_AIController* GetOwningController() const;

    // The world of whoever owns this strategy (its controller, or the
    // coordinator holding it) -- for timers and Blueprint world-context nodes
    virtual UWorld* GetWorld() const override;

    // Whether Loadout has an action that may send RequestType (e.g.
    // ConnectIt_Game_PlacePiece) -- the same question the server's gate asks
    // of a real request. Says nothing about use limits. Game thread only.
    UFUNCTION(BlueprintPure, Category = "AI")
    static bool LoadoutGrantsRequestType(const UActionLoadoutDataAsset* Loadout, FGameplayTag RequestType);

    // A decision with a single move -- the common case
    UFUNCTION(BlueprintPure, Category = "AI")
    static FConnectItAIDecision MakeSingleMoveDecision(const FInstancedStruct& Operation, const FString& Summary);

    // The kinds of move (request types) this strategy can ever offer. Used to
    // check an AI profile: its loadout and its strategy should overlap.
    // Empty (the default) = not declared, no check.
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "AI")
    FGameplayTagContainer GetSupportedRequestTypes() const;
    virtual FGameplayTagContainer GetSupportedRequestTypes_Implementation() const { return FGameplayTagContainer(); }

    // Whether Operation (a board operation struct) is a kind of move the AI
    // may make right now (Context.AvailableRequestTypes). False if it isn't a
    // board operation.
    UFUNCTION(BlueprintPure, Category = "AI")
    static bool IsOperationAvailable(const FConnectItAIDecisionContext& Context, const FInstancedStruct& Operation);

protected:

    // Decide a move for Context, then call FinishDecision -- immediately or
    // later. Default: logs that the strategy implements nothing, finishes
    // with no move.
    UFUNCTION(BlueprintNativeEvent, Category = "AI")
    void BeginDecision(const FConnectItAIDecisionContext& Context);

    // Abandon the decision in progress (stop async work). Default: nothing.
    UFUNCTION(BlueprintNativeEvent, Category = "AI")
    void CancelDecision();

    // Hand the decision back. Ignored unless a decision is active.
    UFUNCTION(BlueprintCallable, Category = "AI")
    void FinishDecision(const FConnectItAIDecision& Decision);

    UFUNCTION(BlueprintPure, Category = "AI")
    bool IsDecisionActive() const { return ActiveDecisionId != INDEX_NONE; }

    // For async work: capture the id when starting, and only finish if it is
    // still the active one when the work completes.
    int32 GetActiveDecisionId() const { return ActiveDecisionId; }
    bool IsDecisionActive(int32 DecisionId) const
    {
        return DecisionId != INDEX_NONE && DecisionId == ActiveDecisionId;
    }

private:

    int32 ActiveDecisionId = INDEX_NONE;
    int32 NextDecisionId = 0;
};
