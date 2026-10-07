// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ConnectIt_Structs.h"
#include "ConnectIt_WinCondition.generated.h"

// A win condition's "has anyone won?" test as plain, thread-safe C++ -- for
// code that can't call the win-condition UObject (it may be Blueprint, and
// must stay on the game thread), i.e. the AI's background MinMax search. Made
// by the win condition itself (IConnectIt_WinCondition::MakeSearchWinCheck)
// so the AI always tests exactly the level's real rule. Read-only once made.
struct CONNECTIT_API FConnectItWinCheck
{
    virtual ~FConnectItWinCheck() = default;

    // The faction slot that has won on Board, or INDEX_NONE if nobody has.
    virtual int32 GetWinningFaction(const FConnectItBoardState& Board) const = 0;
};

// This class does not need to be modified.
UINTERFACE(Blueprintable, BlueprintType)
class UConnectIt_WinCondition : public UInterface
{
    GENERATED_BODY()
};

// Pluggable win-condition strategy -- whether the game has been won after
// the current move's scoring has been applied. Assigned via
// UConnectIt_BoardRules::WinConditionRule (TObjectPtr<UObject>, EditAnywhere
// Instanced, MustImplement). UConnectIt_ScoreThresholdWinCondition is the
// default implementation; other conditions (e.g. occupying a set of tiles
// instead of a score threshold) can be added as new implementers without
// touching UConnectIt_BoardRequestMediator.
class CONNECTIT_API IConnectIt_WinCondition
{
    GENERATED_BODY()

public:

    // Called after scoring has been applied for the current move.
    // Implementations set MutableState.bGameOver / WinningFactionSlot when
    // their condition is met.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt|WinCondition")
    void CheckWinCondition(UPARAM(ref) FConnectItBoardState& MutableState);

    // Score needed to win under this condition, for UI progress display
    // (see FConnectItBoardState::TargetScore, which this feeds). Returns 0
    // for conditions that aren't score-based (e.g. occupying a set of
    // tiles) -- UI treats 0 as "no meaningful progress bar." Default
    // implementation returns 0; score-based conditions override it.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt|WinCondition")
    float GetTargetScore() const;

    // Change the score needed to win (e.g. a target chosen in the main
    // menu's match setup). Returns false if this condition isn't score-based
    // and ignored it. Called on the per-match copy of the rule, never the
    // level config asset's own instance.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt|WinCondition")
    bool SetTargetScore(float NewTargetScore);

    // C++ only. This condition's win test as a thread-safe snapshot (current
    // settings baked in) for the AI's search; called on the game thread. Null
    // (the default, and what Blueprint win conditions get) = the AI can't
    // detect this kind of win, so its search never sees the game end.
    virtual TSharedPtr<const FConnectItWinCheck, ESPMode::ThreadSafe> MakeSearchWinCheck() const
    {
        return nullptr;
    }
};
