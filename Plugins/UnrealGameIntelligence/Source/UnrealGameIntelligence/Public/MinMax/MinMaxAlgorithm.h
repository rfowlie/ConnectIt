// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinMaxUtility.h"


namespace MinMaxAlgorithm
{
	template<typename TNode>
	using TFIsGameOverDelegate = TFunction<bool(const TNode&)>;

	template<typename TNode>
	using TFEvaluateDelegate = TFunction<int32(const TNode&)>;

	// The requirements clause enforces CMinMaxNode at the call site
	// If TNode does not satisfy the concept you get a clear readable error
	template<typename TNode> requires c_min_max_node<TNode>
	static int32 Solve(
		const TNode& Node,
		bool bIsMaximisingPlayer,
		TFIsGameOverDelegate<TNode> IsGameOver,
		TFEvaluateDelegate<TNode> Evaluate)
	{
		const TArray<TNode>& Children = Node.GetChildren();

		if (Children.IsEmpty() || IsGameOver(Node))
		{
			return Evaluate(Node);
		}

		if (bIsMaximisingPlayer)
		{
			int32 MaxEval = TNumericLimits<int32>::Min();
			for (const TNode& Child : Children)
			{
				MaxEval = FMath::Max(MaxEval, Solve(Child, false, IsGameOver, Evaluate));
			}
			return MaxEval;
		}
		else
		{
			int32 MinEval = TNumericLimits<int32>::Max();
			for (const TNode& Child : Children)
			{
				MinEval = FMath::Min(MinEval, Solve(Child, true, IsGameOver, Evaluate));
			}
			return MinEval;
		}
	}
}

// // Decoupled MinMax — works with any node type that exposes a Children array
// // Game specific logic is injected via delegates keeping the algorithm pure
// template<typename TNode> requires c_min_max_node<TNode>
// struct TMinMaxAlgorithm
// {
// 	// Returns true if the node is a terminal game state
// 	using FIsGameOverDelegate = TFunction<bool(const TNode&)>;
//
// 	// Returns the static evaluation score for a terminal node
// 	using FEvaluateDelegate = TFunction<int32(const TNode&)>;
//
// 	static int32 Solve(
// 		const TNode& Node,
// 		bool bIsMaximisingPlayer,
// 		FIsGameOverDelegate IsGameOver,
// 		FEvaluateDelegate Evaluate)
// 	{
// 		TArray<TNode> Children = Node.GetChildren();
// 		
// 		// Terminal condition — leaf node or game over state
// 		if (Children.IsEmpty() || IsGameOver(Node))
// 		{
// 			return Evaluate(Node);
// 		}
//
// 		if (bIsMaximisingPlayer)
// 		{
// 			int32 MaxEval = TNumericLimits<int32>::Min(); // Bug fix — was INT_MAX
// 			for (const TNode& Child : Children)
// 			{
// 				const int32 Eval = Solve(Child, false, IsGameOver, Evaluate);
// 				MaxEval = FMath::Max(MaxEval, Eval);
// 			}
// 			
// 			return MaxEval;
// 		}
// 		else
// 		{
// 			int32 MinEval = TNumericLimits<int32>::Max();
// 			for (const TNode& Child : Children)
// 			{
// 				const int32 Eval = Solve(Child, true, IsGameOver, Evaluate); // Bug fix — was !bIsMaximisingPlayer which is correct but explicit is cleaner
// 				MinEval = FMath::Min(MinEval, Eval); // Bug fix — was FMath::Max
// 			}
// 			
// 			return MinEval;
// 		}
// 	}
// };


