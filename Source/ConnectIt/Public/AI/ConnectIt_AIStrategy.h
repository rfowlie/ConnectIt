// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "ConnectIt_Structs.h"
#include "Board/Rules/ConnectIt_WinCondition.h"
#include "ConnectIt_AIStrategy.generated.h"

class AConnectIt_AIController;


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

    // From the level's scoring rule (UConnectIt_BoardRules::GetMinimumConnectLength)
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    int32 ConnectLength = 4;

    // From the level's win condition; 0 = not score-based. Information for
    // strategies -- searches should use WinCheck to detect wins.
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    float WinScoreThreshold = 0.f;

    // C++ only: the level's win condition as a thread-safe test (see
    // IConnectIt_WinCondition::MakeSearchWinCheck). Null = this win condition
    // can't be tested off the game thread.
    TSharedPtr<const FConnectItWinCheck, ESPMode::ThreadSafe> WinCheck;
};

// What a strategy decided: one board-change request, without the parts the
// controller fills in (ActionTag, FactionID). The controller finds the
// loadout action that produces RequestType and submits it as that action.
USTRUCT(BlueprintType)
struct FConnectItAIDecision
{
    GENERATED_BODY()

    // e.g. ConnectIt_Game_PlacePiece. Left empty = no move.
    UPROPERTY(BlueprintReadWrite, Category = "AI")
    FGameplayTag RequestType;

    // The request's payload struct, e.g. FConnectItRequestPlacePiece
    UPROPERTY(BlueprintReadWrite, Category = "AI")
    FInstancedStruct Payload;

    // Free text for the log -- why this move
    UPROPERTY(BlueprintReadWrite, Category = "AI")
    FString Summary;

    bool HasMove() const { return RequestType.IsValid(); }
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
