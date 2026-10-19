// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ConnectIt_AIDirector.generated.h"

class AConnectIt_AIController;
class UActionLoadoutDataAsset;
class UConnectIt_AIStrategy;
class UConnectIt_AISelector;

// The AI opponent of a match, and how it changes as the match goes on. Lives
// on a match preset (UConnectIt_MatchPreset::AIDirector): one per adventure
// level, or one per way of playing against the AI (e.g. a Classic director).
//
// It owns the AI's setup:
//   * StartingLoadout -- what the AI may do (its actions), from the start;
//   * Strategies -- what proposes moves, in priority order, configured inline;
//   * Selector -- how the move is picked from the strategies' answers;
//   * the think-time settings.
// The base class as it comes is the classic opponent: one MinMax strategy,
// First Available. Subclass it in Blueprint to change the setup during a
// match from its events (or anything else it can reach through the world).
//
// Examples (see the recipe "Direct the AI during a match"):
//   * alternate between two loadouts every turn;
//   * new moves, a smarter strategy or a harsher selector when either side
//     reaches a win checkpoint;
//   * a "tell": set the AI's next loadout when the opponent's turn starts --
//     it replicates, so the UI can show what's coming.
//
// React to committed state (these events, the board's state changing), never
// to visual event tags: visuals are paced by the presentation queue and can
// lag the real state by seconds.
//
// Server-only, like the AI controller it belongs to (each controller
// duplicates the preset's director). It never picks a move itself.
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CONNECTIT_API UConnectIt_AIDirector : public UObject
{
    GENERATED_BODY()

public:

    explicit UConnectIt_AIDirector(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // --- Setup ---

    // The AI's loadout when the match starts (change it later with Set AI Loadout)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Director")
    TObjectPtr<UActionLoadoutDataAsset> StartingLoadout = nullptr;

    // What proposes moves, highest priority first. All run at once each
    // decision; each strategy here is the live one (change the list with Set
    // AI Strategies).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "AI|Director")
    TArray<TObjectPtr<UConnectIt_AIStrategy>> Strategies;

    // Picks the move from the strategies' answers (change it with Set AI Selector)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "AI|Director")
    TObjectPtr<UConnectIt_AISelector> Selector = nullptr;

    // How long the strategies may think per decision; 0 = until every
    // strategy has finished
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Director", meta = (ClampMin = 0.0, Units = "s"))
    float ThinkTimeBudgetSeconds = 2.f;

    // The share of the think time each strategy is told it has (a search
    // stops there), leaving the rest for collecting and selecting
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Director", meta = (ClampMin = 0.01, ClampMax = 1.0))
    float StrategyBudgetShare = 0.8f;

    // The AI never moves sooner than this after it starts deciding, so a
    // fast decision still reads as "thinking"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Director", meta = (ClampMin = 0.0, Units = "s"))
    float MinThinkSeconds = 0.6f;

    // --- Events ---

    // The AI has joined the match, before its first turn: set up here
    UFUNCTION(BlueprintNativeEvent, Category = "AI|Director")
    void OnMatchStarted();
    virtual void OnMatchStarted_Implementation() {}

    // The AI's turn has started, before it decides anything
    UFUNCTION(BlueprintNativeEvent, Category = "AI|Director")
    void OnAITurnStarted(int32 TurnNumber);
    virtual void OnAITurnStarted_Implementation(int32 TurnNumber) {}

    // The AI's turn is over (ended, timed out or skipped)
    UFUNCTION(BlueprintNativeEvent, Category = "AI|Director")
    void OnAITurnEnded(int32 TurnNumber);
    virtual void OnAITurnEnded_Implementation(int32 TurnNumber) {}

    // The opponent's turn has started. A loadout set here applies at once
    // and replicates: the place for a tell of the AI's next move.
    UFUNCTION(BlueprintNativeEvent, Category = "AI|Director")
    void OnOpponentTurnStarted(int32 TurnNumber);
    virtual void OnOpponentTurnStarted_Implementation(int32 TurnNumber) {}

    // A faction completed one of its win groups (not its last). bIsAI: it
    // was the AI's. GroupIndex / GroupName: which one, as in the preset's
    // rules (PlayerWinGroups or AIWinGroups).
    UFUNCTION(BlueprintNativeEvent, Category = "AI|Director")
    void OnWinCheckpointReached(int32 Faction, bool bIsAI, int32 GroupIndex, FName GroupName);
    virtual void OnWinCheckpointReached_Implementation(int32 Faction, bool bIsAI, int32 GroupIndex, FName GroupName) {}

    // --- For the controller ---

    // Adopts strategy / selector changes made since the last decision. Called
    // by the controller as each decision starts.
    void ApplyPendingChanges();

    // The controller this director belongs to (its outer)
    UFUNCTION(BlueprintPure, Category = "AI|Director")
    AConnectIt_AIController* GetOwningController() const;

    virtual UWorld* GetWorld() const override;

protected:

    // Gives the AI a different loadout. During the AI's own turn the change
    // waits until that turn ends (so it can't hand itself extra moves
    // mid-turn); otherwise it applies at once. Uses restart from the new
    // loadout's values.
    UFUNCTION(BlueprintCallable, Category = "AI|Director")
    void SetAILoadout(UActionLoadoutDataAsset* Loadout);

    // The loadout the AI has now
    UFUNCTION(BlueprintPure, Category = "AI|Director")
    UActionLoadoutDataAsset* GetAILoadout() const;

    // Replaces the strategies, from the AI's next decision on. The templates
    // are copied (e.g. instanced strategy variables on this director).
    UFUNCTION(BlueprintCallable, Category = "AI|Director")
    void SetAIStrategies(const TArray<UConnectIt_AIStrategy*>& StrategyTemplates);

    // Replaces the selector, from the AI's next decision on. The template is copied.
    UFUNCTION(BlueprintCallable, Category = "AI|Director")
    void SetAISelector(UConnectIt_AISelector* SelectorTemplate);

private:

    UPROPERTY()
    TArray<TObjectPtr<UConnectIt_AIStrategy>> PendingStrategies;
    bool bHasPendingStrategies = false;

    UPROPERTY()
    TObjectPtr<UConnectIt_AISelector> PendingSelector = nullptr;
};
