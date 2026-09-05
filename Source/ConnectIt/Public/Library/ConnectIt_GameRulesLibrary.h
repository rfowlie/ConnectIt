// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConnectIt_GameRulesLibrary.generated.h"

// NOTE: ConnectIt_Score_Max/IsGameOver below happen to share a value with
// UConnectIt_ScoreThresholdWinCondition::WinScoreThreshold (both default to
// 100) but are NOT the same source of truth as production UI's win-target
// display -- see FConnectItBoardState::TargetScore /
// UConnectIt_BoardRulesComponent::GetTargetScore for that. This constant is
// purely internal to the MinMax AI heuristic (ConnectIt_MinMaxManager/
// ConnectIt_MinMaxTreeBuilder use it to clamp/normalise search-node scores,
// and as their own terminal-node win check on detached hypothetical board
// states that were never on a live UConnectIt_BoardRulesComponent to begin
// with). Deliberately left as its own compile-time constant rather than
// collapsed into TargetScore -- doing so would mean the AI's search
// normalisation reads a runtime, potentially-mutated value instead of a
// fixed bound, which is a heuristic-behaviour change, not a UI-bridge one.
// If the two are ever meant to track each other, that's a follow-up for
// whoever owns the MinMax code, not this constant's own comment.
UCLASS()
class CONNECTIT_API UConnectIt_GameRulesLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	static constexpr int32 ConnectIt_Score_Max = 100;

	UFUNCTION(BlueprintCallable, Category = "ConnectIt")
	static bool IsGameOver(const TArray<float>& ScoreBoard);
};
