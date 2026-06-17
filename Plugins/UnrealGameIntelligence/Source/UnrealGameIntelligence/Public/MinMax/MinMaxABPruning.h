// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinMaxUtility.h"


namespace MinMaxABPruningAlgorithm
{
    template<typename TNode>
    using FIsGameOverDelegate = TFunction<bool(const TNode&)>;

    template<typename TNode>
    using FEvaluateDelegate = TFunction<int32(const TNode&)>;

    // Internal implementation — carries alpha beta state through recursion
    template<typename TNode> requires c_min_max_node<TNode>
    static int32 Solve(
        const TNode& Node,
        bool bIsMaximisingPlayer,
        int32 Alpha,
        int32 Beta,
        const FIsGameOverDelegate<TNode>& IsGameOver,
        const FEvaluateDelegate<TNode>& Evaluate)
    {
        const TArray<TNode>& Children = Node.GetChildren();

        // Terminal condition — leaf node or game over
        if (Children.IsEmpty() || IsGameOver(Node))
        {
            return Evaluate(Node);
        }

        if (bIsMaximisingPlayer)
        {
            int32 MaxEval = TNumericLimits<int32>::Min();

            for (const TNode& Child : Children)
            {
                int32 Eval = Solve(Child, false, Alpha, Beta, IsGameOver, Evaluate);
                MaxEval = FMath::Max(MaxEval, Eval);

                // Update alpha — best guaranteed score for maximiser
                Alpha = FMath::Max(Alpha, Eval);

                // Beta cutoff — minimiser already has a better option elsewhere
                // No point evaluating remaining siblings
                if (Beta <= Alpha)
                {
                    break;
                }
            }

            return MaxEval;
        }
        else
        {
            int32 MinEval = TNumericLimits<int32>::Max();

            for (const TNode& Child : Children)
            {
                int32 Eval = Solve(Child, true, Alpha, Beta, IsGameOver, Evaluate);
                MinEval = FMath::Min(MinEval, Eval);

                // Update beta — best guaranteed score for minimiser
                Beta = FMath::Min(Beta, Eval);

                // Alpha cutoff — maximiser already has a better option elsewhere
                // No point evaluating remaining siblings
                if (Beta <= Alpha)
                {
                    break;
                }
            }

            return MinEval;
        }
    }
}
