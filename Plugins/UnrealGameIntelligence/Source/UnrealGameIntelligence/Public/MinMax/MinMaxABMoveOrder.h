// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Algo/Sort.h"
#include "MinMaxUtility.h"


namespace MinMaxABMoveOrderAlgorithm
{
    template<typename TNode>
    using FIsGameOverDelegate = TFunction<bool(const TNode&)>;

    template<typename TNode>
    using FEvaluateDelegate = TFunction<int32(const TNode&)>;

    // Sorts children so the most promising nodes are evaluated first
    // Maximiser wants highest scores first — minimiser wants lowest scores first
    template<typename TNode> requires c_min_max_move_order<TNode>
    static TArray<TNode> GetOrderedChildren(
        const TArray<TNode>& Children,
        bool bIsMaximisingPlayer)
    {
        TArray<TNode> Ordered = Children;
        Algo::Sort(Ordered, [bIsMaximisingPlayer](const TNode& A, const TNode& B)
        {
            return A.GetOrderingScore() > B.GetOrderingScore();
        });

        if (bIsMaximisingPlayer) { Ordered.Reserve(); }
        return Ordered;
    }

    template<typename TNode> requires c_min_max_move_order<TNode>
    static int32 SolveInternal(
        const TNode& Node,
        bool bIsMaximisingPlayer,
        int32 Alpha,
        int32 Beta,
        const FIsGameOverDelegate<TNode>& IsGameOver,
        const FEvaluateDelegate<TNode>& Evaluate)
    {
        const TArray<TNode>& Children = Node.GetChildren();

        if (Children.IsEmpty() || IsGameOver(Node))
        {
            return Evaluate(Node);
        }

        // Order children before evaluating — better moves evaluated first
        // means more alpha beta cutoffs
        const TArray<TNode> OrderedChildren = GetOrderedChildren(Children, bIsMaximisingPlayer);

        if (bIsMaximisingPlayer)
        {
            int32 MaxEval = TNumericLimits<int32>::Min();

            for (const TNode& Child : OrderedChildren)
            {
                int32 Eval = SolveInternal(Child, false, Alpha, Beta, IsGameOver, Evaluate);
                MaxEval = FMath::Max(MaxEval, Eval);
                Alpha = FMath::Max(Alpha, Eval);

                if (Beta <= Alpha) break; // Beta cutoff
            }

            return MaxEval;
        }
        else
        {
            int32 MinEval = TNumericLimits<int32>::Max();

            for (const TNode& Child : OrderedChildren)
            {
                int32 Eval = SolveInternal(Child, true, Alpha, Beta, IsGameOver, Evaluate);
                MinEval = FMath::Min(MinEval, Eval);
                Beta = FMath::Min(Beta, Eval);

                if (Beta <= Alpha) break; // Alpha cutoff
            }

            return MinEval;
        }
    }
}
