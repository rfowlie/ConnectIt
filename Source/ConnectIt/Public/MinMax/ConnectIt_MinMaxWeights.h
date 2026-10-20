// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_MinMaxWeights.generated.h"


// The MinMax AI's judgement, as numbers a designer tunes on
// UConnectIt_AIStrategy_MinMax. Each weight scales one factor; the factors
// themselves, and how they are combined, are plain code in
// FConnectItMinMaxRules (EvaluateState / EvaluateMove). A new factor is a new
// weight here plus its logic there.


// What the AI values in an unfinished position: the search's score for a
// position is the weighted sum of these factors. (A finished game is never
// scored by these -- win/loss is checked first.) 0 turns a factor off.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItMinMaxEvaluationWeights
{
    GENERATED_BODY()

    // Real points: my score minus the opponent's. Kept large so one point of
    // real score outweighs any realistic Line Potential swing.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Evaluation")
    float ScoreDifference = 1000.f;

    // Lines in the making: for every connect-length window holding only one
    // faction's pieces (and empty, active tiles), pieces² × the window's
    // total multiplier. Mine minus the opponent's. Only means something when
    // the match scores by lines.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Evaluation")
    float LinePotential = 10.f;

    bool IsAllZero() const { return ScoreDifference == 0.f && LinePotential == 0.f; }
};

// Which moves the search tries first: a move's ordering score is the weighted
// sum of these factors. Only affects speed (better guesses first = more
// pruning), never which move wins. 0 turns a factor off.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItMinMaxOrderingWeights
{
    GENERATED_BODY()

    // The score multiplier of the tile the move puts a piece on
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Move Ordering")
    float TileMultiplier = 10.f;

    // How many neighbouring tiles hold a piece (either faction) -- where
    // lines get built and blocked
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Move Ordering")
    float AdjacentPieces = 5.f;
};
