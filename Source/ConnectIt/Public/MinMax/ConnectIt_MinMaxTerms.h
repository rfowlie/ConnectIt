// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "ConnectIt_MinMaxTerms.generated.h"

class FConnectItMinMaxRules;


// The building blocks of the MinMax AI's judgement. UConnectIt_AIStrategy_MinMax
// holds two editor lists of these -- evaluation terms and ordering terms --
// that designers pick, order and weight in the Details panel. New kinds of
// term are new C++ structs deriving from the bases below.
//
// Terms run INSIDE the search, on a background thread, up to millions of times
// per decision. So a term must be a pure function of its inputs: no UObject
// references, no mutable state, nothing slow. (That is also why these are
// plain structs rather than UObjects or Blueprint: the search copies them once
// on the game thread and then only calls them.)


// One factor in how good a position is.
// The search's score for a position = sum of Weight * Evaluate(...) over the
// strategy's evaluation terms (after the rules' own win/loss check).
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItMinMaxEvalTerm
{
    GENERATED_BODY()

    virtual ~FConnectItMinMaxEvalTerm() = default;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Evaluation")
    float Weight = 1.f;

    // How good Board is for Side (positive = good for Side). Usually "mine
    // minus theirs" so the term is symmetric.
    virtual float Evaluate(
        const FConnectItMinMaxRules& Rules, const FConnectItBoardState& Board, int32 Side) const
    {
        return 0.f;
    }
};

// Real points: my score minus the opponent's.
USTRUCT(BlueprintType, meta = (DisplayName = "Score Difference"))
struct CONNECTIT_API FConnectItMinMaxEvalTerm_ScoreDifference : public FConnectItMinMaxEvalTerm
{
    GENERATED_BODY()

    // One point of real score should outweigh any realistic line-potential swing
    FConnectItMinMaxEvalTerm_ScoreDifference() { Weight = 1000.f; }

    virtual float Evaluate(
        const FConnectItMinMaxRules& Rules, const FConnectItBoardState& Board, int32 Side) const override;
};

// Lines in the making: for every connect-length window holding only one
// faction's pieces (and empty, active tiles), pieces² × the window's total
// multiplier. Mine minus the opponent's.
USTRUCT(BlueprintType, meta = (DisplayName = "Line Potential"))
struct CONNECTIT_API FConnectItMinMaxEvalTerm_LinePotential : public FConnectItMinMaxEvalTerm
{
    GENERATED_BODY()

    FConnectItMinMaxEvalTerm_LinePotential() { Weight = 10.f; }

    virtual float Evaluate(
        const FConnectItMinMaxRules& Rules, const FConnectItBoardState& Board, int32 Side) const override;

    // One faction's side of the sum
    static float LinePotential(
        const FConnectItMinMaxRules& Rules, const FConnectItBoardState& Board, int32 Side);
};


// One factor in how promising a move looks BEFORE searching it. Only affects
// speed (better guesses first = more pruning), never which move wins.
// Ordering score = sum of Weight * Score(...) over the strategy's ordering terms.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItMinMaxOrderTerm
{
    GENERATED_BODY()

    virtual ~FConnectItMinMaxOrderTerm() = default;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Move Ordering")
    float Weight = 1.f;

    // How promising placing on TileIndex (an index into Board.TileDataArray)
    // looks for Side.
    virtual float Score(
        const FConnectItMinMaxRules& Rules, const FConnectItBoardState& Board,
        int32 TileIndex, int32 Side) const
    {
        return 0.f;
    }
};

// The tile's multiplier.
USTRUCT(BlueprintType, meta = (DisplayName = "Tile Multiplier"))
struct CONNECTIT_API FConnectItMinMaxOrderTerm_TileMultiplier : public FConnectItMinMaxOrderTerm
{
    GENERATED_BODY()

    FConnectItMinMaxOrderTerm_TileMultiplier() { Weight = 10.f; }

    virtual float Score(
        const FConnectItMinMaxRules& Rules, const FConnectItBoardState& Board,
        int32 TileIndex, int32 Side) const override;
};

// How many neighbouring tiles hold a piece (either faction) -- where lines get
// built and blocked.
USTRUCT(BlueprintType, meta = (DisplayName = "Adjacent Pieces"))
struct CONNECTIT_API FConnectItMinMaxOrderTerm_AdjacentPieces : public FConnectItMinMaxOrderTerm
{
    GENERATED_BODY()

    FConnectItMinMaxOrderTerm_AdjacentPieces() { Weight = 5.f; }

    virtual float Score(
        const FConnectItMinMaxRules& Rules, const FConnectItBoardState& Board,
        int32 TileIndex, int32 Side) const override;
};
